#ifndef _MT_STATISTICS_H
#define _MT_STATISTICS_H

#include <chrono>
#include <map>

#include "audio/Audio_DataWindow.h"
#include "helper/HL_Optional.hpp"
#include "helper/HL_Statistics.h"

#include "jrtplib/src/rtptimeutilities.h"
#include "jrtplib/src/rtppacket.h"

using std::experimental::optional;

namespace MT
{

template<typename T>
struct StreamStats
{
    T mChunk;
    T mTotal;
};


class JitterStatistics
{
public:
    void process(jrtplib::RTPPacket* packet, int samplerate);
    TestResult<float> get() const  { return mJitter; }
    float getMaxDelta() const      { return mMaxDelta; }

protected:
    // Jitter calculation
    jrtplib::RTPTime mReceiveTime = jrtplib::RTPTime(0,0);

    // Last timestamp from packet in units
    uint32_t mReceiveTimestamp = 0;

    // It is classic jitter value in units
    optional<float> mLastJitter;

    // Some statistics for jitter value in seconds
    TestResult<float> mJitter;

    // Maximal delta in seconds
    float mMaxDelta = 0.0f;
};

class Statistics
{
public:
    size_t      mReceived,        // Received traffic in bytes
                mSent,            // Sent traffic in bytes
                mReceivedRtp,     // Number of received rtp packets
                mSentRtp,         // Number of sent rtp packets
                mReceivedRtcp,    // Number of received rtcp packets
                mSentRtcp,        // Number of sent rtcp packets
                mDuplicatedRtp,   // Number of received duplicated rtp packets
                mOldRtp,          // Number of late rtp packets
                mPacketLoss,      // Number of lost packets
                mPacketDropped,   // Number of dropped packets (due to time unsync when playing)б
                mIllegalRtp;      // Number of rtp packets with bad payload type

    TestResult<float> mDecodingInterval, // Average interval on call to packet decode
                      mDecodeRequested,  // Average amount of requested audio frames to play
                      mPacketInterval;   // Average interval between packet adding to jitter buffer

    int         mLoss[128];       // Every item is number of loss of corresping length
    size_t      mAudioTime;       // Decoded/found time in milliseconds
    uint16_t    mSsrc;            // Last known SSRC ID in a RTP stream
    ice::NetworkAddress mRemotePeer; // Last known remote RTP address

    // AMR codec bitrate switch counter
    int       mBitrateSwitchCounter;

    std::string mCodecName;

    float     mJitter;          // Jitter

    TestResult<float> mRttDelay; // RTT delay

    // Timestamp when first RTP packet has arrived
    optional<std::chrono::system_clock::time_point> mFirstRtpTime;

    std::map<int, int> mCodecCount;          // Stats on used codecs

    // It is to calculate network MOS
    void calculateBurstr(double* burstr, double* loss) const;
    double calculateMos(double maximalMos) const;

    Statistics();
    ~Statistics();
    void reset();

    Statistics& operator += (const Statistics& src);
    Statistics& operator -= (const Statistics& src);

    float mNetworkMos = 0.0;
#if defined(USE_PVQA_LIBRARY) && !defined(PVQA_SERVER)
    float mPvqaMos = 0.0;
    std::string mPvqaReport;
#endif

    std::string toString() const;
};

} // end of namespace MT


#endif
