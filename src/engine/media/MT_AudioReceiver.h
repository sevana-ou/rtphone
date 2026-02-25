/* Copyright(C) 2007-2025 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_AUDIO_RECEIVER_H
#define __MT_AUDIO_RECEIVER_H

#include "../engine_config.h"
#include "MT_Stream.h"
#include "MT_CodecList.h"
#include "MT_CngHelper.h"

#include "../helper/HL_Sync.h"

#include "jrtplib/src/rtppacket.h"
#include "jrtplib/src/rtpsourcedata.h"
#include "../audio/Audio_DataWindow.h"
#include "../audio/Audio_Resampler.h"

#include <optional>
#include <chrono>
using namespace std::chrono_literals;

namespace MT
{
using jrtplib::RTPPacket;
class RtpBuffer
{
public:
    // Owns rtp packet data
    class Packet
    {
    public:
        Packet(const std::shared_ptr<RTPPacket>& packet, std::chrono::milliseconds timelen, int samplerate);
        std::shared_ptr<RTPPacket> rtp() const;

        std::chrono::milliseconds timelength() const;
        int samplerate() const;

        const std::vector<short>& pcm() const;
        std::vector<short>& pcm();

        const std::chrono::microseconds& timestamp() const;
        std::chrono::microseconds& timestamp();

    protected:
        std::shared_ptr<RTPPacket> mRtp;
        std::chrono::milliseconds mTimelength = 0ms;
        int mSamplerate = 0;
        std::vector<short> mPcm;
        std::chrono::microseconds mTimestamp = 0us;
    };

    struct FetchResult
    {
        enum class Status
        {
            RegularPacket,
            Gap,
            NoPacket
        };

        Status mStatus = Status::NoPacket;
        std::shared_ptr<Packet> mPacket;

        std::string toString() const
        {
            switch (mStatus)
            {
                case Status::RegularPacket: return "packet";
                case Status::Gap:           return "gap";
                case Status::NoPacket:      return "empty";
            }
        }
    };

    RtpBuffer(Statistics& stat);
    ~RtpBuffer();

    unsigned ssrc() const;
    void setSsrc(unsigned ssrc);

    void setHigh(std::chrono::milliseconds t);
    std::chrono::milliseconds high() const;

    void setLow(std::chrono::milliseconds t);
    std::chrono::milliseconds low() const;

    void setPrebuffer(std::chrono::milliseconds t);
    std::chrono::milliseconds prebuffer() const;

    int getNumberOfReturnedPackets() const;
    int getNumberOfAddPackets() const;

    std::chrono::milliseconds findTimelength();
    int getCount() const;

    // Returns false if packet was not add - maybe too old or too new or duplicate
    std::shared_ptr<Packet> add(const std::shared_ptr<RTPPacket>& packet, std::chrono::milliseconds timelength, int rate);

    typedef std::vector<std::shared_ptr<Packet>> ResultList;
    typedef std::shared_ptr<ResultList> PResultList;

    FetchResult fetch();
    
protected:
    unsigned    mSsrc = 0;
    std::chrono::milliseconds   mHigh = std::chrono::milliseconds(RTP_BUFFER_HIGH),
                                mLow = std::chrono::milliseconds(RTP_BUFFER_LOW),
                                mPrebuffer = std::chrono::milliseconds(RTP_BUFFER_PREBUFFER);
    int         mReturnedCounter = 0,
                mAddCounter = 0;

    mutable Mutex mGuard;
    typedef std::vector<std::shared_ptr<Packet>> PacketList;
    PacketList mPacketList;
    Statistics& mStat;
    bool mFirstPacketWillGo = true;
    jrtplib::RTPSourceStats mRtpStats;
    std::shared_ptr<Packet> mFetchedPacket;
    std::optional<uint32_t> mLastSeqno;
    std::optional<jrtplib::RTPTime> mLastReceiveTime;

    // To calculate average interval between packet add. It is close to jitter but more useful in debugging.
    float mLastAddTime = 0.0f;
};

class Receiver
{
public:
    Receiver(Statistics& stat);
    virtual ~Receiver();

protected:
    Statistics& mStat;
};

class AudioReceiver: public Receiver
{
public:
    AudioReceiver(const CodecList::Settings& codecSettings, Statistics& stat);
    ~AudioReceiver();
    
    // Update codec settings
    void                    setCodecSettings(const CodecList::Settings& codecSettings);
    CodecList::Settings&    getCodecSettings();

    // Returns false when packet is rejected as illegal. codec parameter will show codec which will be used for decoding.
    // Lifetime of pointer to codec is limited by lifetime of AudioReceiver (it is container).
    bool add(const std::shared_ptr<jrtplib::RTPPacket>& p, Codec** codec = nullptr);

    struct DecodeOptions
    {
        bool mResampleToMainRate = true;                // Resample all decoded audio to AUDIO_SAMPLERATE
        bool mFillGapByCNG = false;                     // Use CNG information if available
        bool mSkipDecode = false;                       // Don't do decode, just dry run - fetch packets, remove them from the jitter buffer
        std::chrono::milliseconds mElapsed = 0ms;       // How much milliseconds should be decoded; zero value means "decode just next packet from the buffer"
    };

    struct DecodeResult
    {
        enum class Status
        {
            Ok,        // Decoded ok
            Skip,      // Just no data - emit silence instead
            BadPacket  // Error happened during the decode
        };

        Status          mStatus = Status::Ok;
        int             mSamplerate = 0;
        int             mChannels = 0;
    };

    DecodeResult getAudioTo(Audio::DataWindow& output, DecodeOptions options);

    // Looks for codec by payload type
    Codec* findCodec(int payloadType);
    RtpBuffer&  getRtpBuffer() { return mBuffer; }

    // Returns size of AudioReceiver's instance in bytes (including size of all data + codecs + etc.)
    int getSize() const;

    // Returns timelength for given packet
    int timelengthFor(jrtplib::RTPPacket& p);

    // Return samplerate for given packet
    int samplerateFor(jrtplib::RTPPacket& p);

protected:
    RtpBuffer                           mBuffer;                // Jitter buffer itself
    CodecMap                            mCodecMap;
    PCodec                              mCodec;
    int                                 mFrameCount = 0;
    CodecList::Settings                 mCodecSettings;
    CodecList                           mCodecList;
    JitterStatistics                    mJitterStats;
    std::shared_ptr<RtpBuffer::Packet>  mCngPacket;
    CngDecoder                          mCngDecoder;
    size_t                              mDTXSamplesToEmit = 0;   // How much silence (or CNG) should be emited before next RTP packet gets into the action

    // Already decoded data that can be retrieved without actual decoding - it may happen because of getAudioTo() may be limited by time interval
    Audio::DataWindow mAvailable;

    // Temporary buffer to hold decoded data (it is better than allocate data on stack)
    int16_t mDecodedFrame[MT_MAX_DECODEBUFFER];
    size_t mDecodedLength = 0;

    // Buffer to hold data converted to stereo/mono; there is multiplier 2 as it can be stereo audio
    int16_t mConvertedFrame[MT_MAX_DECODEBUFFER * 2];
    size_t mConvertedLength = 0;

    // Buffer to hold data resampled to AUDIO_SAMPLERATE
    int16_t mResampledFrame[MT_MAX_DECODEBUFFER];
    size_t mResampledLength = 0;

    // Last packet time length
    int mLastPacketTimeLength = 0;
    std::optional<uint32_t> mLastPacketTimestamp;

    int mFailedCount = 0;
    Audio::Resampler  mResampler8,
                      mResampler16,
                      mResampler32,
                      mResampler48;

    Audio::PWavFileWriter mDecodedDump;

    std::optional<std::chrono::steady_clock::time_point> mDecodeTimestamp; // Time last call happened to codec->decode()

    float mIntervalSum = 0.0f;
    int mIntervalCount = 0;

    // Zero rate will make audio mono but resampling will be skipped
    void makeMonoAndResample(int rate, int channels);

    // Resamples, sends to analysis, writes to dump and queues to output decoded frames from mDecodedFrame
    void processDecoded(Audio::DataWindow& output, DecodeOptions options);
    void produceSilence(std::chrono::milliseconds length, Audio::DataWindow& output, DecodeOptions options);
    void produceCNG(std::chrono::milliseconds length, Audio::DataWindow& output, DecodeOptions options);

    // Calculate bitrate switch statistics for AMR codecs
    void updateAmrCodecStats(Codec* c);

    DecodeResult decodeGapTo(Audio::DataWindow& output, DecodeOptions options);
    DecodeResult decodePacketTo(Audio::DataWindow& output, DecodeOptions options, const std::shared_ptr<RtpBuffer::Packet>& p);
    DecodeResult decodeEmptyTo(Audio::DataWindow& output, DecodeOptions options);
};

class DtmfReceiver: public Receiver
{
private:
    char mEvent = 0;
    bool mEventEnded = false;
    std::chrono::milliseconds mEventStart = 0ms;
    std::function<void(char)> mCallback;

public:
    DtmfReceiver(Statistics& stat);
    ~DtmfReceiver();

    void add(const std::shared_ptr<RTPPacket>& p);
    void setCallback(std::function<void(char tone)> callback);
};
}

#endif
