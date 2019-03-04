#ifndef _MT_STATISTICS_H
#define _MT_STATISTICS_H

#include <chrono>
#include <map>

#include "audio/Audio_DataWindow.h"
#include "helper/HL_Optional.hpp"
#include "jrtplib/src/rtptimeutilities.h"
#include "jrtplib/src/rtppacket.h"

#include "MT_SevanaMos.h"

using std::experimental::optional;

namespace MT
{
template<typename T>
struct Average
{
    int mCount = 0;
    T mSum = 0;
    T getAverage() const
    {
        if (!mCount)
            return 0;
        return mSum / mCount;
    }

    void process(T value)
    {
        mCount++;
        mSum += value;
    }
};

template<typename T, int minimum = 100000, int maximum = 0>
struct ProbeStats
{
    T mMin = minimum;
    T mMax = maximum;
    Average<T> mAverage;
    T mCurrent = minimum;

    void process(T value)
    {
        if (mMin > value)
            mMin = value;
        if (mMax < value)
            mMax = value;
        mCurrent = value;
        mAverage.process(value);
    }

    bool isInitialized() const
    {
        return mAverage.mCount > 0;
    }

    T getCurrent() const
    {
        if (isInitialized())
            return mCurrent;
        else
            return 0;
    }
};


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
    ProbeStats<double> get() const  { return mJitter; }
    double getMaxDelta() const      { return mMaxDelta; }

protected:
    // Jitter calculation
    jrtplib::RTPTime mReceiveTime = jrtplib::RTPTime(0,0);
    uint32_t mReceiveTimestamp = 0;
    optional<double> mLastJitter;
    ProbeStats<double> mJitter;
    double mMaxDelta = 0.0;
    uint64_t mPrevRxTimestamp = 0;
    uint64_t mPrevArrival = 0;
    uint64_t mPrevTransit = 0;
};

class Statistics
{
public:
    int       mReceived,        // Received traffic in bytes
    mSent,            // Sent traffic in bytes
    mReceivedRtp,     // Number of received rtp packets
    mSentRtp,         // Number of sent rtp packets
    mReceivedRtcp,    // Number of received rtcp packets
    mSentRtcp,        // Number of sent rtcp packets
    mDuplicatedRtp,   // Number of received duplicated rtp packets
    mOldRtp,          // Number of late rtp packets
    mPacketLoss,      // Number of lost packets
    mIllegalRtp;      // Number of rtp packets with bad payload type
    int       mLoss[128];       // Every item is number of loss of corresping length
    int       mAudioTime;       // Decoded/found time in milliseconds
    uint16_t  mSsrc;            // Last known SSRC ID in a RTP stream
    ice::NetworkAddress mRemotePeer; // Last known remote RTP address

#if defined(USE_AMR_CODEC)
    int       mBitrateSwitchCounter;
#endif

    std::string mCodecName;

    float     mJitter;          // Jitter

    ProbeStats<double> mRttDelay; // RTT delay

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

    std::string toShortString() const;
};

} // end of namespace MT


#endif
