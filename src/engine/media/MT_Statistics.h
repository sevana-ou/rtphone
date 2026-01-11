#ifndef _MT_STATISTICS_H
#define _MT_STATISTICS_H

#include <chrono>
#include <map>
#include <optional>
#include <array>

#include "audio/Audio_DataWindow.h"
#include "helper/HL_Optional.hpp"
#include "helper/HL_Statistics.h"
#include "helper/HL_Types.h"

#include "jrtplib/src/rtptimeutilities.h"
#include "jrtplib/src/rtppacket.h"


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
    jrtplib::RTPTime        mReceiveTime = jrtplib::RTPTime(0,0);

    // Last timestamp from packet in units
    uint32_t                mReceiveTimestamp = 0;

    // It is classic jitter value in units
    std::optional<float>    mLastJitter;

    // Some statistics for jitter value in seconds
    TestResult<float>       mJitter;

    // Maximal delta in seconds
    float                   mMaxDelta = 0.0f;
};

struct PacketLossEvent
{
    // This is extended sequence numbers (not the raw uint16_t seqno)
    uint32_t    mStartSeqno = 0,
                mEndSeqno = 0;
    int         mGap = 0;
    std::chrono::microseconds mTimestamp;
};

class Statistics
{
public:
    size_t                      mReceived = 0,        // Received traffic in bytes
                                mSent = 0,            // Sent traffic in bytes
                                mReceivedRtp = 0,     // Number of received rtp packets
                                mSentRtp = 0,         // Number of sent rtp packets
                                mReceivedRtcp = 0,    // Number of received rtcp packets
                                mSentRtcp = 0,        // Number of sent rtcp packets
                                mDuplicatedRtp = 0,   // Number of received duplicated rtp packets
                                mOldRtp = 0,          // Number of late rtp packets
                                mPacketLoss = 0,      // Number of lost packets
                                mPacketDropped = 0,   // Number of dropped packets (due to time unsync when playing)б
                                mIllegalRtp = 0;      // Number of rtp packets with bad payload type

    TestResult<float>           mDecodingInterval,      // Average interval on call to packet decode
                                mDecodeRequested,       // Average amount of requested audio frames to play
                                mPacketInterval;        // Average interval between packet adding to jitter buffer

    std::array<float, 128>      mLoss = {0};            // Every item is number of loss of corresping length
    size_t                      mAudioTime = 0;         // Decoded/found time in milliseconds
    size_t                      mDecodedSize = 0;       // Number of decoded bytes
    uint16_t                    mSsrc = 0;              // Last known SSRC ID in a RTP stream
    ice::NetworkAddress         mRemotePeer;            // Last known remote RTP address

    // AMR codec bitrate switch counter
    int                         mBitrateSwitchCounter = 0;
    std::string                 mCodecName;
    float                       mJitter = 0.0f;         // Jitter
    TestResult<float>           mRttDelay;              // RTT delay

    // Timestamp when first RTP packet has arrived
    std::optional<timepoint_t>  mFirstRtpTime;

    std::map<int, int>          mCodecCount;            // Stats on used codecs

    std::vector<PacketLossEvent> mPacketLossTimeline;   // Packet loss timeline

    // It is to calculate network MOS
    void calculateBurstr(double* burstr, double* loss) const;
    double calculateMos(double maximalMos) const;

    Statistics();
    ~Statistics();
    void reset();

    Statistics& operator += (const Statistics& src);
    Statistics& operator -= (const Statistics& src);

    float mNetworkMos = 0.0f;

    std::string toString() const;
};

} // end of namespace MT


#endif
