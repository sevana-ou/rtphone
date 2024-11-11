/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_CODEC_H
#define __AUDIO_CODEC_H

#include "../engine_config.h"
#include <map>
#include "MT_Codec.h"
#include "../audio/Audio_Resampler.h"
#include "../helper/HL_Pointer.h"
#include "webrtc/ilbcfix/ilbc.h"
#include "webrtc/isac/isacfix.h"
#include "webrtc/g711/g711_interface.h"

extern "C"
{
#include "libgsm/gsm.h"
#include "g722/g722.h"
}

#include "libg729/g729_typedef.h"
#include "libg729/g729_ld8a.h"

#if defined(USE_OPUS_CODEC)
#   include "opus.h"
#endif

namespace MT 
{
class G729Codec: public Codec
{
protected:
    CodState* mEncoder = nullptr;
    DecState* mDecoder = nullptr;
    void decodeFrame(const uint8_t* rtp, int16_t* pcm);

public:
    class G729Factory: public Factory
    {
    public:
        const char* name() override;
        int channels() override;
        int samplerate() override;
        int payloadType() override;

#if defined(USE_RESIP_INTEGRATION)
        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
#endif
        PCodec create() override;
    };
    G729Codec();
    ~G729Codec() override;

    const char* name() override;
    int pcmLength() override;
    int rtpLength() override;
    int frameTime() override;
    int samplerate() override;
    int channels() override;
    int encode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int decode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int plc(int lostFrames, void* output, int outputCapacity) override;
};

#if defined(USE_OPUS_CODEC)
class OpusCodec: public Codec
{
protected:
    OpusEncoder        *mEncoderCtx;
    OpusDecoder        *mDecoderCtx;
    int mPTime, mSamplerate, mChannels;
    // Audio::SpeexResampler mDecodeResampler;
    int mDecoderChannels;
public:
    struct Params
    {
        bool mUseDtx, mUseInbandFec, mStereo;
        int mPtime, mTargetBitrate, mExpectedPacketLoss;

        Params();
#if defined(USE_RESIP_INTEGRATION)
        resip::Data toString() const;
        void parse(const resip::Data& params);
#endif
    };

    class OpusFactory: public Factory
    {
    protected:
        int mSamplerate, mChannels, mPType;
        OpusCodec::Params mParams;
        typedef std::vector<PCodec> CodecList;
        CodecList mCodecList;

    public:
        OpusFactory(int samplerate, int channels, int ptype);

        const char* name() override;
        int channels() override;
        int samplerate() override;
        int payloadType() override;
#if defined(USE_RESIP_INTEGRATION)
        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
#endif
        PCodec create() override;
    };

    OpusCodec(int samplerate, int channels, int ptime);
    ~OpusCodec();
    void applyParams(const Params& params);

    const char* name();
    int pcmLength();
    int rtpLength();
    int frameTime();
    int samplerate();
    int channels();
    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity);
};
#endif

class IlbcCodec: public Codec
{
protected:
    int                 mPacketTime;          /// Single frame time (20 or 30 ms)
    iLBC_encinst_t*     mEncoderCtx;
    iLBC_decinst_t*     mDecoderCtx;
    
public:
    class IlbcFactory: public Factory
    {
    protected:
        int mPtime;
        int mPType20ms, mPType30ms;

    public:

        IlbcFactory(int ptype20ms, int ptype30ms);
        const char* name();
        int samplerate();
        int payloadType();
#if defined(USE_RESIP_INTEGRATION)
        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction);
        void create(CodecMap& codecs);
#endif
        PCodec create();
    };

    IlbcCodec(int packetTime);
    virtual ~IlbcCodec();
    const char* name();
    int pcmLength();
    int rtpLength();
    int frameTime();
    int samplerate();
    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity);
};

class G711Codec: public Codec
{
public:
    class AlawFactory: public Factory
    {
    public:
        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();
    };

    class UlawFactory: public Factory
    {
    public:
        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();
    };

    enum
    {
        ALaw = 0,   /// a-law codec type
        ULaw = 1    /// u-law codec type
    };
    
    G711Codec(int type);
    ~G711Codec();
    
    const char* name();
    int    pcmLength();
    int    frameTime();
    int    rtpLength();
    int    samplerate();

    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostSamples, void* output, int outputCapacity);

protected:
    int mType;    /// Determines if it is u-law or a-law codec. Its value is ALaw or ULaw.
};

class IsacCodec: public Codec
{
protected:
    int                       mSamplerate;
    ISACFIX_MainStruct*       mEncoderCtx;
    ISACFIX_MainStruct*       mDecoderCtx;
public:
    class IsacFactory16K: public Factory
    {
    public:
        IsacFactory16K(int ptype);
        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();

    protected:
        int mPType;
    };
    
    class IsacFactory32K: public Factory
    {
    public:
        IsacFactory32K(int ptype);

        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();

    protected:
        int mPType;
    };

    IsacCodec(int sampleRate);
    ~IsacCodec();
    
    const char* name();
    int pcmLength();
    int rtpLength();
    int frameTime();
    int samplerate();

    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity);
};



/// GSM MIME name
#define GSM_MIME_NAME "gsm"

/// Optional GSM SIP attributes
#define GSM_SIP_ATTR ""

/// GSM codec single frame time in milliseconds
#define GSM_AUDIOFRAME_TIME 20

/// GSM codec single RTP frame size in bytes
#define GSM_RTPFRAME_SIZE_33 33
#define GSM_RTPFRAME_SIZE_32 32
#define GSM_RTPFRAME_SIZE_31 31

/// GSM payload type
#define GSM_PAYLOAD_TYPE 3

/// GSM bitrate (bits/sec)
#define GSM_BITRATE 13000


/*!
   * GSM codec wrapper. Based on implementation located in libgsm directory.
   * @see IMediaCodec
   */
class GsmCodec: public Codec
{
public:
    enum class Type
    {
        Bytes_33,
        Bytes_32,
        Bytes_31,
        Bytes_65
    };
protected:
    struct gsm_state * mGSM; /// Pointer to codec context
    Type mCodecType;

public:
    class GsmFactory: public Factory
    {
    protected:
        int mPayloadType;
        Type mCodecType;

    public:
        GsmFactory(Type codecType = Type::Bytes_33, int pt = GSM_PAYLOAD_TYPE);

        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();
    };

    /*! Default constructor. Initializes codec context's pointer to NULL. */
    GsmCodec(Type codecType);

    /*! Destructor. */
    virtual ~GsmCodec();

    const char* name();
    int pcmLength();
    int rtpLength();
    int frameTime();
    int samplerate();

    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity);
};

/// GSM MIME name
#define G722_MIME_NAME "g722"

/// Optional GSM SIP attributes
#define G722_SIP_ATTR ""

/// GSM codec single frame time in milliseconds
#define G722_AUDIOFRAME_TIME 20

/// GSM codec single RTP frame size in bytes
#define G722_RTPFRAME_SIZE 80

/// GSM payload type
#define G722_PAYLOAD_TYPE 9

/// GSM bitrate (bits/sec)
#define G722_BITRATE 64000

class G722Codec: public Codec
{
protected:
    void* mEncoder;
    void* mDecoder;

public:
    class G722Factory: public Factory
    {
    public:
        G722Factory();

        const char* name();
        int samplerate();
        int payloadType();
        PCodec create();
    };
    G722Codec();
    virtual ~G722Codec();

    const char* name();
    int pcmLength();
    int rtpLength();
    int frameTime();
    int samplerate();

    int encode(const void* input, int inputBytes, void* output, int outputCapacity);
    int decode(const void* input, int inputBytes, void* output, int outputCapacity);
    int plc(int lostFrames, void* output, int outputCapacity);

    //unsigned GetSamplerate() { return 16000; }
};

class GsmHrCodec: public Codec
{
protected:
    void* mDecoder;

public:
    class GsmHrFactory: public Factory
    {
    protected:
        int mPtype;

    public:
        GsmHrFactory(int ptype);

        const char* name() override;
        int samplerate() override;
        int payloadType() override;
        PCodec create() override;
    };

    GsmHrCodec();
    ~GsmHrCodec() override;

    const char* name() override;
    int pcmLength() override;
    int rtpLength() override;
    int frameTime() override;
    int samplerate() override;

    int encode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int decode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int plc(int lostFrames, void* output, int outputCapacity) override;
};
}

#endif
