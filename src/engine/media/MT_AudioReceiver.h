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
    enum class FetchResult
    {
        RegularPacket,
        Gap,
        NoPacket
    };

    // Owns rtp packet data
    class Packet
    {
    public:
        Packet(const std::shared_ptr<RTPPacket>& packet, int timelen, int rate);
        std::shared_ptr<RTPPacket> rtp() const;

        int timelength() const;
        int rate() const;

        const std::vector<short>& pcm() const;
        std::vector<short>& pcm();

        const std::chrono::microseconds& timestamp() const;
        std::chrono::microseconds& timestamp();

    protected:
        std::shared_ptr<RTPPacket> mRtp;
        int mTimelength = 0,
            mRate = 0;
        std::vector<short> mPcm;
        std::chrono::microseconds mTimestamp = 0us;
    };

    RtpBuffer(Statistics& stat);
    ~RtpBuffer();

    unsigned ssrc() const;
    void setSsrc(unsigned ssrc);

    void setHigh(int milliseconds);
    int high() const;

    void setLow(int milliseconds);
    int low() const;

    void setPrebuffer(int milliseconds);
    int prebuffer() const;

    int getNumberOfReturnedPackets() const;
    int getNumberOfAddPackets() const;

    int findTimelength();
    int getCount() const;

    // Returns false if packet was not add - maybe too old or too new or duplicate
    std::shared_ptr<Packet> add(std::shared_ptr<RTPPacket> packet, int timelength, int rate);

    typedef std::vector<std::shared_ptr<Packet>> ResultList;
    typedef std::shared_ptr<ResultList> PResultList;

    FetchResult fetch(ResultList& rl);
    
protected:
    unsigned    mSsrc = 0;
    int         mHigh = RTP_BUFFER_HIGH,
                mLow = RTP_BUFFER_LOW,
                mPrebuffer = RTP_BUFFER_PREBUFFER;
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

    // Returns false when there is no rtp data from jitter
    /*enum DecodeOptions
    {
        DecodeOptions_ResampleToMainRate = 0,
        DecodeOptions_DontResample = 1,
        DecodeOptions_FillCngGap = 2,
        DecodeOptions_SkipDecode = 4
    };*/

    struct DecodeOptions
    {
        bool mResampleToMainRate = true;
        bool mFillGapByCNG = false;
        bool mSkipDecode = false;
    };

    enum DecodeResult
    {
        DecodeResult_Ok,        // Decoded ok
        DecodeResult_Skip,      // Just no data - emit silence instead
        DecodeResult_BadPacket  // Error happened during the decode
    };

    DecodeResult getAudio(Audio::DataWindow& output, DecodeOptions options = {.mResampleToMainRate = true, .mFillGapByCNG = false, .mSkipDecode = false}, int* rate = nullptr);

    // Looks for codec by payload type
    Codec* findCodec(int payloadType);
    RtpBuffer& getRtpBuffer() { return mBuffer; }

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
    std::shared_ptr<jrtplib::RTPPacket> mCngPacket;
    CngDecoder                          mCngDecoder;
    size_t                              mDTXSamplesToEmit = 0;   // How much silence (or CNG) should be emited before next RTP packet gets into the action

    // Buffer to hold decoded data
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
    Audio::Resampler  mResampler8, mResampler16, mResampler32, mResampler48;

    Audio::PWavFileWriter mDecodedDump;

    std::optional<std::chrono::steady_clock::time_point> mLastDecodeTimestamp; // Time last call happened to codec->decode()

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

    DecodeResult decodeGap(Audio::DataWindow& output, DecodeOptions options);
    DecodeResult decodePacket(const RtpBuffer::ResultList& rl, Audio::DataWindow& output, DecodeOptions options, int* rate = nullptr);
    DecodeResult decodeNone(Audio::DataWindow& output, DecodeOptions options);
};

class DtmfReceiver: public Receiver
{
public:
    DtmfReceiver(Statistics& stat);
    ~DtmfReceiver();

    void add(std::shared_ptr<RTPPacket> p);
};
}

#endif
