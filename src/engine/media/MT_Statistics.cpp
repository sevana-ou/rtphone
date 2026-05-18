#include <cmath>
#include <cctype>
#include <cstring>
#include <iostream>
#include <sstream>

#include "MT_Statistics.h"
#define LOG_SUBSYSTEM "media"

using namespace MT;

namespace
{

// Per-codec impairment parameters (Ie, Bpl) from ITU-T G.113 / G.107.
// clockRate == 0 means "any".
struct MosCodecEntry { const char* mName; unsigned mClockRate; double mIe; double mBpl; };

constexpr MosCodecEntry kMosCodecTable[] = {
    { "PCMU",       8000,   0.0, 25.0 },
    { "PCMA",       8000,   0.0, 25.0 },
    { "G722",       8000,  13.0, 21.0 },
    { "G7221",     16000,  13.0, 21.0 },
    { "G7221",     32000,  13.0, 21.0 },
    { "G729",       8000,  11.0, 19.0 },
    { "G729A",      8000,  11.0, 19.0 },
    { "G729AB",     8000,  11.0, 19.0 },
    { "G723",       8000,  15.0, 16.0 },
    { "iLBC",       8000,  11.0, 18.0 },
    { "GSM",        8000,  20.0, 10.0 },
    { "AMR",        8000,   5.0, 10.0 },
    { "AMR-WB",    16000,   7.0, 10.0 },
    { "speex",      8000,  15.0, 20.0 },
    { "speex",     16000,  10.0, 20.0 },
    { "speex",     32000,  10.0, 20.0 },
    { "opus",      48000,   5.0, 25.0 },

    // EVS — no published G.113 value. Using AMR-WB-family Bpl with a
    // conservative Ie that matches typical commercial VQM tools for EVS
    // Primary ~13.2 kbps WB.
    { "EVS",       16000,   5.0, 10.0 },
};

constexpr double kMosDefaultIe  = 0.0;
constexpr double kMosDefaultBpl = 25.0;

bool iequals(const std::string& a, const char* b)
{
    const size_t n = std::strlen(b);
    if (a.size() != n) return false;
    for (size_t i = 0; i < n; ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i])))
            return false;
    return true;
}

void resolveMosCodecParams(const std::string& codecName, double& ie, double& bpl)
{
    ie  = kMosDefaultIe;
    bpl = kMosDefaultBpl;
    if (codecName.empty())
        return;

    // Map known codec-name aliases before looking up Ie/Bpl entries.
    std::string lookup = codecName;
    if (iequals(lookup, "GSM-06.10"))
        lookup = "GSM";

    for (const auto& e: kMosCodecTable)
        if (iequals(lookup, e.mName))
        {
            ie  = e.mIe;
            bpl = e.mBpl;
            return;
        }
}

} // anonymous namespace

void JitterStatistics::process(jrtplib::RTPPacket* packet, int rate)
{
    // RFC 3550 §A.8 jitter. Two guards:
    //
    //   1. Update only when the new packet is exactly one sequence number
    //      after the previous in-sequence packet. Skipping this check across
    //      packet-loss gaps inflates jitter; skipping out-of-order packets
    //      entirely (the previous behaviour) under-reports it.
    //   2. Ignore the first few in-sequence samples while transit time
    //      settles after call setup.
    constexpr uint32_t kIgnoreFirstPackets = 5;

    const uint32_t timestamp = packet->GetTimestamp();
    const uint32_t extSeqno  = packet->GetExtendedSequenceNumber();
    const jrtplib::RTPTime receiveTime = packet->GetReceiveTime();

    // First packet: just stash state.
    if (!mLastJitter)
    {
        mReceiveTime      = receiveTime;
        mReceiveTimestamp = timestamp;
        mLastExtSeqno     = extSeqno;
        mLastJitter       = 0.0;
        mPacketsProcessed = 1;
        return;
    }

    // RFC 3550 §A.8: only adjacent packets contribute to jitter.
    // Out-of-order, duplicate, and post-loss packets are skipped silently —
    // but state must still advance so the *next* in-sequence pair works.
    const bool adjacent = mLastExtSeqno && (extSeqno == mLastExtSeqno.value() + 1);

    if (!adjacent)
    {
        // Reset the transit reference if a discontinuity (loss / reorder)
        // happened, restarting from the latest known good packet.
        if (mLastExtSeqno && extSeqno > mLastExtSeqno.value())
        {
            mReceiveTime      = receiveTime;
            mReceiveTimestamp = timestamp;
            mLastExtSeqno     = extSeqno;
        }
        return;
    }

    // RTP FAQ: also skip when timestamp is unchanged (multi-packet frame, dup).
    if (timestamp == mReceiveTimestamp)
    {
        mLastExtSeqno = extSeqno;
        return;
    }

    // Wrap-safe signed delta on the 32-bit RTP timestamp:
    // transit = arrival - rtp_ts; d = transit - prev_transit (signed 32-bit).
    const int32_t timestampDelta = static_cast<int32_t>(timestamp - mReceiveTimestamp);
    const int64_t receiveDelta   =
        static_cast<int64_t>(receiveTime.GetDouble() * rate) -
        static_cast<int64_t>(mReceiveTime.GetDouble()  * rate);
    const int64_t delta = receiveDelta - timestampDelta;

    // Save state for the next pair regardless of warmup.
    mReceiveTime      = receiveTime;
    mReceiveTimestamp = timestamp;
    mLastExtSeqno     = extSeqno;
    ++mPacketsProcessed;

    // Skip the first N in-sequence samples while transit time settles.
    if (mPacketsProcessed <= kIgnoreFirstPackets)
        return;

    const float deltaSec = static_cast<float>(std::fabs(static_cast<double>(delta) / rate));
    if (deltaSec > mMaxDelta)
        mMaxDelta = deltaSec;

    // J = J + (|D| - J) / 16
    mLastJitter = mLastJitter.value() +
                  (std::fabs(static_cast<double>(delta)) - mLastJitter.value()) / 16.0;

    mJitter.process(mLastJitter.value() / static_cast<float>(rate));
}


// ---------------------------- Statistics ------------------------------------


Statistics::Statistics()
{}

Statistics::~Statistics()
{}

void Statistics::calculateBurstr(double* burstr, double* lossr) const
{
    int lost = 0; // Total packet lost
    for (const auto& item: mPacketLossTimeline)
        lost += item.mGap;
    int bursts = mPacketLossTimeline.size(); // number of events

    // for (const auto& entry: mLoss)
    // {
    //     lost += entry.first * entry.second;
    //     bursts += entry.second;
    // }

    if (lost < 5)
    {
        // ignore such small packet loss
        *lossr = *burstr = 0;
        return;
    }

    if (mReceivedRtp > 0 && bursts > 0)
    {
        *burstr = ((double)lost / (double)bursts) * (1.0 - (double)lost / (double)mReceivedRtp);
        if (*burstr < 1.0)
            *burstr = 1.0;
    }
    else
        *burstr = 0;

    if (mReceivedRtp > 0)
        *lossr = (double)((double)lost / (double)mReceivedRtp);
    else
        *lossr = 0;
}

double Statistics::calculateMos() const
{
    // Network MOS via the simplified ITU-T G.107 E-Model:
    //
    //   d_oneway = rtt/2 + jitter + jb_delay         (ms)
    //   Id       = 0.024*d + 0.11*max(0, d - 177.3)
    //   Ie_eff   = Ie + (95 - Ie) * Ppl / (Ppl + Bpl)         (BurstR=1)
    //   R        = 93.2 - Id - Ie_eff                          (clamped to [0,100])
    //   MOS      = 1 + 0.035*R + 7e-6*R*(R-60)*(100-R)        (clamped >= 1)
    //
    // Ie/Bpl are looked up from a per-codec table; safe defaults are used
    // when the codec is unknown.

    if (mReceivedRtp < 10)
        return 0.0;

    // Loss percent is computed as lost / (lost + received).
    const uint64_t expected = static_cast<uint64_t>(mReceivedRtp) +
                              static_cast<uint64_t>(mPacketLoss);
    const double Ppl = expected > 0
                       ? static_cast<double>(mPacketLoss) * 100.0 / static_cast<double>(expected)
                       : 0.0;

    double Ie = kMosDefaultIe, Bpl = kMosDefaultBpl;
    resolveMosCodecParams(mCodecName, Ie, Bpl);
    if (Bpl <= 0.0)
        Bpl = 1.0;

    // mRttDelay and mJitter are stored in seconds. jb_delay is unknown at
    // this layer, so it is treated as zero.
    const double rttMs    = static_cast<double>(mRttDelay.average()) * 1000.0;
    const double jitterMs = static_cast<double>(mJitter) * 1000.0;
    const double d        = rttMs / 2.0 + jitterMs;

    double Id = 0.024 * d;
    if (d > 177.3)
        Id += 0.11 * (d - 177.3);

    const double Ie_eff = Ie + (95.0 - Ie) * Ppl / (Ppl + Bpl);

    double R = 93.2 - Id - Ie_eff;
    if (R < 0.0)   R = 0.0;
    if (R > 100.0) R = 100.0;

    double mos;
    if (R == 0.0)
        mos = 1.0;
    else
        mos = 1.0 + 0.035 * R + 7e-6 * R * (R - 60.0) * (100.0 - R);

    if (mos < 1.0) mos = 1.0;
    return mos;
}

Statistics& Statistics::operator += (const Statistics& src)
{
    mReceived       += src.mReceived;
    mSent           += src.mSent;
    mReceivedRtp    += src.mReceivedRtp;
    mSentRtp        += src.mSentRtp;
    mReceivedRtcp   += src.mReceivedRtcp;
    mSentRtcp       += src.mSentRtcp;
    mDuplicatedRtp  += src.mDuplicatedRtp;
    mOldRtp         += src.mOldRtp;
    mPacketLoss     += src.mPacketLoss;
    mPacketDropped  += src.mPacketDropped;
    mAudioTime      += src.mAudioTime;


    for (auto codecStat: src.mCodecCount)
    {
        if (mCodecCount.find(codecStat.first) == mCodecCount.end())
            mCodecCount[codecStat.first] = codecStat.second;
        else
            mCodecCount[codecStat.first] += codecStat.second;
    }

    mJitter             = src.mJitter;
    mRttDelay           = src.mRttDelay;
    mDecodingInterval   = src.mDecodingInterval;
    mDecodeRequested    = src.mDecodeRequested;

    if (!src.mCodecName.empty())
        mCodecName = src.mCodecName;

    // Find minimal
    if (mFirstRtpTime)
    {
        if (src.mFirstRtpTime)
        {
            if (mFirstRtpTime.value() > src.mFirstRtpTime.value())
                mFirstRtpTime = src.mFirstRtpTime;
        }
    }
    else
    if (src.mFirstRtpTime)
        mFirstRtpTime = src.mFirstRtpTime;

    mBitrateSwitchCounter   += src.mBitrateSwitchCounter;
    mRemotePeer             = src.mRemotePeer;
    mSsrc                   = src.mSsrc;

    for (const auto& [addr, counts]: src.mPerDestination)
    {
        auto& dst = mPerDestination[addr];
        dst.mSentRtp       += counts.mSentRtp;
        dst.mSentRtcp      += counts.mSentRtcp;
        dst.mSentBytes     += counts.mSentBytes;
        dst.mReceivedRtp   += counts.mReceivedRtp;
        dst.mReceivedRtcp  += counts.mReceivedRtcp;
        dst.mReceivedBytes += counts.mReceivedBytes;
    }

    return *this;
}

Statistics& Statistics::operator -= (const Statistics& src)
{
    mReceived           -= src.mReceived;
    mSent               -= src.mSent;
    mReceivedRtp        -= src.mReceivedRtp;
    mIllegalRtp         -= src.mIllegalRtp;
    mSentRtp            -= src.mSentRtp;
    mReceivedRtcp       -= src.mReceivedRtcp;
    mSentRtcp           -= src.mSentRtcp;
    mDuplicatedRtp      -= src.mDuplicatedRtp;
    mOldRtp             -= src.mOldRtp;
    mPacketLoss         -= src.mPacketLoss;
    mPacketDropped      -= src.mPacketDropped;
    mAudioTime          -= src.mAudioTime;

    for (auto codecStat: src.mCodecCount)
    {
        if (mCodecCount.find(codecStat.first) != mCodecCount.end())
            mCodecCount[codecStat.first] -= codecStat.second;
    }

    for (const auto& [addr, counts]: src.mPerDestination)
    {
        auto it = mPerDestination.find(addr);
        if (it == mPerDestination.end())
            continue;
        it->second.mSentRtp       -= counts.mSentRtp;
        it->second.mSentRtcp      -= counts.mSentRtcp;
        it->second.mSentBytes     -= counts.mSentBytes;
        it->second.mReceivedRtp   -= counts.mReceivedRtp;
        it->second.mReceivedRtcp  -= counts.mReceivedRtcp;
        it->second.mReceivedBytes -= counts.mReceivedBytes;
    }

    return *this;
}


std::string Statistics::toString() const
{
    std::ostringstream oss;
    oss << "Received: "             << mReceivedRtp
        << ", lost: "               << mPacketLoss
        << ", dropped: "            << mPacketDropped
        << ", sent: "               << mSentRtp
        << ", decoding interval: "  << mDecodingInterval.average()
        << ", decode requested: "   << mDecodeRequested.average()
        << ", packet interval: "    << mPacketInterval.average();

    for (const auto& [addr, counts]: mPerDestination)
    {
        oss << "; peer " << addr.toBriefStdString()
            << " sent rtp="  << counts.mSentRtp
            << "/rtcp="      << counts.mSentRtcp
            << "/bytes="     << counts.mSentBytes
            << ", received rtp=" << counts.mReceivedRtp
            << "/rtcp="          << counts.mReceivedRtcp
            << "/bytes="         << counts.mReceivedBytes;
    }

    return oss.str();
}
