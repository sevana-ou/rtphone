#include <math.h>

#include "MT_Statistics.h"
#include "audio/Audio_Interface.h"
#include "helper/HL_Log.h"
#define LOG_SUBSYSTEM "Statistics"

using namespace MT;

void JitterStatistics::process(jrtplib::RTPPacket* packet, int rate)
{
    // Get current timestamp and receive time
    uint32_t timestamp = packet->GetTimestamp();
    jrtplib::RTPTime receiveTime = packet->GetReceiveTime();

    if (!mLastJitter.is_initialized())
    {
        // First packet
        mReceiveTime = receiveTime;
        mReceiveTimestamp = timestamp;
        mLastJitter = 0.0;
    }
    else
    {
        // It is in units
        int64_t receiveDelta = int64_t(receiveTime.GetDouble() * rate) - int64_t(mReceiveTime.GetDouble() * rate);

        // Check if packets are ordered ok
        if (timestamp <= mReceiveTimestamp)
            return;

        // Find differences in timestamp
        int64_t timestampDelta = timestamp - mReceiveTimestamp;

        if (!timestampDelta)
            // Skip current packet silently. Most probably it is error in RTP stream like duplicated packet.
            return;

        // Find delta in units
        int64_t delta = receiveDelta - timestampDelta;

        // Update max delta in milliseconds
        float delta_in_seconds = float(fabs(double(delta) / rate));
        if (delta_in_seconds > mMaxDelta)
            mMaxDelta = delta_in_seconds;

        // Update jitter value in units
        mLastJitter = mLastJitter.value() + (fabs(double(delta)) - mLastJitter.value()) / 16.0;
        /*printf("PacketNo: %d, current delta in ms: %f, jitter in ms: %f\n",
               (int)packet->GetSequenceNumber(),
               delta_in_ms,
               float(mLastJitter.value() / (rate / 1000)));*/

        // Save last values
        mReceiveTime = receiveTime;
        mReceiveTimestamp = timestamp;

        // And mJitter are in seconds again
        mJitter.process(mLastJitter.value() / float(rate));
    }
}


// ---------------------------- Statistics ------------------------------------


Statistics::Statistics()
    :mReceived(0), mSent(0), mReceivedRtp(0), mSentRtp(0),
      mReceivedRtcp(0), mSentRtcp(0), mDuplicatedRtp(0), mOldRtp(0), mIllegalRtp(0),
      mPacketLoss(0), mJitter(0.0), mAudioTime(0), mDecodedSize(0), mSsrc(0), mPacketDropped(0)
{
    mBitrateSwitchCounter = 0;
    memset(mLoss, 0, sizeof mLoss);

    // It is to keep track of statistics instance via grep | wc -l
    //ICELogDebug(<< "Create statistics instance.");
}

Statistics::~Statistics()
{
}

void Statistics::reset()
{
    mReceived = 0;
    mSent = 0;
    mReceivedRtp = 0;
    mSentRtp = 0;
    mReceivedRtcp = 0;
    mSentRtcp = 0;
    mDuplicatedRtp = 0;
    mOldRtp = 0;
    mPacketLoss = 0;
    mIllegalRtp = 0;
    mJitter = 0.0;
    mAudioTime = 0;
    mPacketDropped = 0;
    mDecodedSize = 0;

    memset(mLoss, 0, sizeof mLoss);
}

void Statistics::calculateBurstr(double* burstr, double* lossr) const
{
    int lost = 0;
    int bursts = 0;
    for (int i = 0; i < 128; i++)
    {
        lost += i * mLoss[i];
        bursts += mLoss[i];
    }

    if (lost < 5)
    {
        // ignore such small packet loss
        *lossr = *burstr = 0;
        return;
    }

    if (mReceivedRtp > 0 && bursts > 0)
    {
        *burstr = (double)((double)lost / (double)bursts) / (double)(1.0 / (1.0 - (double)lost / (double)mReceivedRtp));
        if (*burstr < 0)
            *burstr = -*burstr;
        else if (*burstr < 1)
            *burstr = 1;
    }
    else
        *burstr = 0;
    //printf("total loss: %d\n", lost);
    if (mReceivedRtp > 0)
        *lossr = (double)((double)lost / (double)mReceivedRtp);
    else
        *lossr = 0;
}

double Statistics::calculateMos(double maximalMos) const
{
    // calculate lossrate and burst rate
    double burstr, lossr;
    calculateBurstr(&burstr, &lossr);

    double r;
    double bpl = 8.47627; //mos = -4.23836 + 0.29873 * r - 0.00416744 * r * r + 0.0000209855 * r * r * r;
    double mos;

    if (mReceivedRtp < 100)
        return 0.0;

    if (lossr == 0.0 || burstr == 0.0)
    {
        return maximalMos;
    }

    if (lossr > 0.5)
        return 1;

    bpl = 17.2647;
    r = 93.2062077233 - 95.0 * (lossr * 100 / (lossr * 100 / burstr + bpl));
    mos = 2.06405 + 0.031738 * r - 0.000356641 * r * r + 2.93143 * pow(10, -6) * r * r * r;
    if (mos < 1)
        return 1;

    if (mos > maximalMos)
        return maximalMos;

    return mos;
}

Statistics& Statistics::operator += (const Statistics& src)
{
    mReceived += src.mReceived;
    mSent += src.mSent;
    mReceivedRtp += src.mReceivedRtp;
    mSentRtp += src.mSentRtp;
    mReceivedRtcp += src.mReceivedRtcp;
    mSentRtcp += src.mSentRtcp;
    mDuplicatedRtp += src.mDuplicatedRtp;
    mOldRtp += src.mOldRtp;
    mPacketLoss += src.mPacketLoss;
    mPacketDropped += src.mPacketDropped;
    mAudioTime += src.mAudioTime;


    for (auto codecStat: src.mCodecCount)
    {
        if (mCodecCount.find(codecStat.first) == mCodecCount.end())
            mCodecCount[codecStat.first] = codecStat.second;
        else
            mCodecCount[codecStat.first] += codecStat.second;
    }

    mJitter = src.mJitter;
    mRttDelay = src.mRttDelay;
    mDecodingInterval = src.mDecodingInterval;
    mDecodeRequested = src.mDecodeRequested;

    if (!src.mCodecName.empty())
        mCodecName = src.mCodecName;

    // Find minimal
    if (mFirstRtpTime.is_initialized())
    {
        if (src.mFirstRtpTime.is_initialized())
        {
            if (mFirstRtpTime.value() > src.mFirstRtpTime.value())
                mFirstRtpTime = src.mFirstRtpTime;
        }
    }
    else
        if (src.mFirstRtpTime.is_initialized())
            mFirstRtpTime = src.mFirstRtpTime;

    mBitrateSwitchCounter += src.mBitrateSwitchCounter;
    mRemotePeer = src.mRemotePeer;
    mSsrc = src.mSsrc;

    return *this;
}

Statistics& Statistics::operator -= (const Statistics& src)
{
    mReceived -= src.mReceived;
    mSent -= src.mSent;
    mReceivedRtp -= src.mReceivedRtp;
    mIllegalRtp -= src.mIllegalRtp;
    mSentRtp -= src.mSentRtp;
    mReceivedRtcp -= src.mReceivedRtcp;
    mSentRtcp -= src.mSentRtcp;
    mDuplicatedRtp -= src.mDuplicatedRtp;
    mOldRtp -= src.mOldRtp;
    mPacketLoss -= src.mPacketLoss;
    mPacketDropped -= src.mPacketDropped;

    mAudioTime -= src.mAudioTime;
    for (auto codecStat: src.mCodecCount)
    {
        if (mCodecCount.find(codecStat.first) != mCodecCount.end())
            mCodecCount[codecStat.first] -= codecStat.second;
    }

    return *this;
}


std::string Statistics::toString() const
{
    std::ostringstream oss;
    oss << "Received: "     << mReceivedRtp
        << ", lost: "       << mPacketLoss
        << ", dropped: "    << mPacketDropped
        << ", sent: "       << mSentRtp
        << ", decoding interval: " << mDecodingInterval.average()
        << ", decode requested: " << mDecodeRequested.average()
        << ", packet interval: " << mPacketInterval.average();

    return oss.str();
}
