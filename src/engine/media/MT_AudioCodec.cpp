/* Copyright(C) 2007-2023 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define NOMINMAX

#include "../engine_config.h"
#include "MT_AudioCodec.h"
#include "MT_CodecList.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Types.h"
#include "../helper/HL_String.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Pointer.h"
#include "../gsmhr/gsmhr.h"

#include "libg729/g729_pre_proc.h"
#include "libg729/g729_util.h"
#include "libg729/g729_ld8a.h"

#include <stdlib.h>
#include <sstream>
#include <assert.h>
#include <memory.h>
#include <string.h>
#include <algorithm>

#define LOG_SUBSYSTEM "Codec"

#ifdef TARGET_LINUX
# define stricmp strcasecmp
#endif

using namespace MT;


// ------------ G729Codec ----------------

const char* G729Codec::G729Factory::name()
{
    return "G729";
}

int G729Codec::G729Factory::channels()
{
    return 1;
}

int G729Codec::G729Factory::samplerate()
{
    return 8000;
}

int G729Codec::G729Factory::payloadType()
{
    return 18;
}

void G729Codec::G729Factory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{}

int G729Codec::G729Factory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    return 0;
}

PCodec G729Codec::G729Factory::create()
{
    return std::make_shared<G729Codec>();
}

G729Codec::G729Codec()
    :mEncoder(nullptr), mDecoder(nullptr)
{
}

G729Codec::~G729Codec()
{
    if (mEncoder)
    {
        free(mEncoder);
        mEncoder = nullptr;
    }

    if (mDecoder)
    {
        free(mDecoder);
        mDecoder = nullptr;
    }
}

Codec::Info G729Codec::info()
{
    return {
        .mName = "G729",
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 10 * 8 * 2,
        .mFrameTime = 10,
        .mRtpLength = 10
    };
}

// static const int SamplesPerFrame = 80;
Codec::EncodeResult G729Codec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    // Create encoder if it is not done yet
    if (!mEncoder)
    {
        mEncoder = Init_Coder_ld8a();
        if (mEncoder)
            Init_Pre_Process(mEncoder);
    }
    size_t result = 0;
    if (mEncoder)
    {
        int nrOfFrames = input.size_bytes() / 160;  // 10ms frames
        Word16 parm[PRM_SIZE];                      // ITU's service buffer
        for (int frameIndex = 0; frameIndex < nrOfFrames; frameIndex++)
        {
            Copy((int16_t*)input.data() + frameIndex * info().mPcmLength / 2, mEncoder->new_speech, info().mPcmLength / 2);
            Pre_Process(mEncoder, mEncoder->new_speech, info().mPcmLength / 2);
            Coder_ld8a(mEncoder, parm);
            Store_Params(parm, output.data() + frameIndex * info().mRtpLength);
            result += info().mRtpLength;
        }
    }
    return {result};
}

Codec::DecodeResult G729Codec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (!mDecoder)
    {
        mDecoder = Init_Decod_ld8a();
        if (mDecoder)
        {
            Init_Post_Filter(mDecoder);
            Init_Post_Process(mDecoder);
        }
    }

    size_t result = 0;
    if (mDecoder)
    {
        // See if there are silence bytes in the end
        bool isSilence = (input.size_bytes() % info().mRtpLength) / 2 != 0;

        // Find number of frames
        int nrOfFrames = input.size_bytes() / info().mRtpLength;
        nrOfFrames = std::min(output.size_bytes() / info().mPcmLength, (size_t)nrOfFrames);

        for (int frameIndex = 0; frameIndex < nrOfFrames; frameIndex++)
            decodeFrame(input.data() + frameIndex * info().mRtpLength, (int16_t*)output.data() + frameIndex * info().mPcmLength);

        result += nrOfFrames * info().mPcmLength;

        if (isSilence && nrOfFrames < output.size_bytes() / info().mPcmLength)
        {
            memset(output.data() + nrOfFrames * info().mPcmLength, 0, info().mPcmLength);
            result += info().mPcmLength;
        }
    }

    return {.mDecoded = result, .mIsCng = false};
}

void G729Codec::decodeFrame(const uint8_t* rtp, int16_t* pcm)
{
    Word16 i;
    Word16 *synth;
    Word16 parm[PRM_SIZE + 1];

    Restore_Params(rtp, &parm[1]);

    synth = mDecoder->synth_buf + M10;

    parm[0] = 1;
    for (i = 0; i < PRM_SIZE; i++)
    {
        if (parm[i + 1] != 0)
        {
            parm[0] = 0;
            break;
        }
    }

    parm[4] = Check_Parity_Pitch(parm[3], parm[4]);

    Decod_ld8a(mDecoder, parm, synth, mDecoder->Az_dec, mDecoder->T2, &mDecoder->bad_lsf);
    Post_Filter(mDecoder, synth, mDecoder->Az_dec, mDecoder->T2);
    Post_Process(mDecoder, synth, L_FRAME);

    for (i = 0; i < pcmLength() / 2; i++)
        pcm[i] = synth[i];

}


size_t G729Codec::plc(int lostFrames, std::span<uint8_t> output)
{
    return 0;
}

// -------------- Opus -------------------
#define OPUS_CODEC_NAME         "OPUS"
#define OPUS_CODEC_RATE         16000
#define OPUS_TARGET_BITRATE     64000
#define OPUS_PACKET_LOSS        10
#define OPUS_CODEC_COMPLEXITY   2

OpusCodec::Params::Params()
    :mUseDtx(false), mUseInbandFec(true), mStereo(true), mPtime(20)
{
    mExpectedPacketLoss = OPUS_PACKET_LOSS;
    mTargetBitrate = OPUS_TARGET_BITRATE;
}

resip::Data OpusCodec::Params::toString() const
{
    std::ostringstream oss;
    //oss << "ptime=" << mPTime << ";";
    if (mUseDtx)
        oss << "usedtx=" << (mUseDtx ? "1" : "0") << ";";
    if (mUseInbandFec)
        oss << "useinbandfec=" << (mUseInbandFec ? "1" : "0") << ";";
    std::string r = oss.str();

    // Erase last semicolon
    if (r.size())
        r.erase(r.size()-1);

    //oss << "stereo=" << (mStereo ? "1" : "0") << ";";
    return resip::Data(r);
}

struct CodecParam
{
    resip::Data mName;
    resip::Data mValue;
};

static void splitParams(const resip::Data& s, std::list<CodecParam>& params)
{
    resip::Data::size_type p = 0;
    while (p != resip::Data::npos)
    {
        resip::Data::size_type p1 = s.find(";", p);

        resip::Data param;
        if (p1 != resip::Data::npos)
        {
            param = s.substr(p, p1 - p);
            p = p1 + 1;
        }
        else
        {
            param = s.substr(p);
            p = resip::Data::npos;
        }


        CodecParam cp;
        // See if there is '=' inside
        resip::Data::size_type ep = param.find("=");
        if (ep != resip::Data::npos)
        {
            cp.mName = param.substr(0, ep);
            cp.mValue = param.substr(ep + 1);
        }
        else
            cp.mName = param;

        params.push_back(cp);
    }
}


void OpusCodec::Params::parse(const resip::Data &params)
{
    // There is opus codec in sdp. Be simple - use the parameters from remote peer to operate
    std::list<CodecParam> parsed;
    splitParams(params, parsed);

    std::list<CodecParam>::const_iterator paramIter;
    for (paramIter = parsed.begin(); paramIter != parsed.end(); ++paramIter)
    {
        if (paramIter->mName == "usedtx")
            mUseDtx = paramIter->mValue == "1";
        else
        if (paramIter->mName == "useinbandfec")
            mUseInbandFec = paramIter->mValue == "1";
        else
        if (paramIter->mName == "stereo")
            mStereo = paramIter->mValue == "1";
        else
        if (paramIter->mName == "ptime")
            mPtime = strx::toInt(paramIter->mValue.c_str(), 20);
    }
}

OpusCodec::OpusFactory::OpusFactory(int samplerate, int channels, int ptype)
{
    mSamplerate = samplerate;
    mChannels = channels;
    mPType = ptype;
}

const char* OpusCodec::OpusFactory::name()
{
    return OPUS_CODEC_NAME;
}

int OpusCodec::OpusFactory::channels()
{
    return mChannels;
}

int OpusCodec::OpusFactory::samplerate()
{
    return mSamplerate;
}

int OpusCodec::OpusFactory::payloadType()
{
    return mPType;
}

void OpusCodec::OpusFactory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    // Put opus codec record
    resip::Codec opus(name(), payloadType(), samplerate());
    if (mParams.mStereo)
        opus.encodingParameters() = "2";
    opus.parameters() = resip::Data(mParams.toString().c_str());

    codecs.push_back(opus);
}

#if defined(TARGET_ANDROID)
# define stricmp strcasecmp
#endif

int OpusCodec::OpusFactory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    resip::SdpContents::Session::Medium::CodecContainer::const_iterator codecIter;
    for (codecIter = codecs.begin(); codecIter != codecs.end(); ++codecIter)
    {
        const resip::Codec& resipCodec = *codecIter;
        // Accept only 48000Hz configurations
        if (stricmp(resipCodec.getName().c_str(), name()) == 0 && resipCodec.getRate() == 48000)
        {
            mParams.parse(resipCodec.parameters());

            // Check number of channels
            mParams.mStereo |= codecIter->encodingParameters() == "2";

            // Here changes must be applied to instantiated codec
            for (CodecList::iterator instanceIter = mCodecList.begin(); instanceIter != mCodecList.end(); ++instanceIter)
            {
                Codec& c = **instanceIter;
                if ((c.channels() == (mParams.mStereo ? 2 : 1)) && resipCodec.getRate() == c.samplerate())
                    dynamic_cast<OpusCodec&>(c).applyParams(mParams);
            }
            return codecIter->payloadType();
        }
    }
    return -1;
}

PCodec OpusCodec::OpusFactory::create()
{
    OpusCodec* result = new OpusCodec(Audio::Format(mSamplerate, mChannels), mParams.mPtime);
    result->applyParams(mParams);
    PCodec c(result);
    mCodecList.push_back(c);

    return c;
}

OpusCodec::OpusCodec(Audio::Format fmt, int ptime)
    :mEncoderCtx(nullptr), mDecoderCtx(nullptr), mChannels(fmt.channels()), mPTime(ptime), mSamplerate(fmt.rate()), mDecoderChannels(0)
{
    int status;
    mEncoderCtx = opus_encoder_create(mSamplerate, mChannels, OPUS_APPLICATION_VOIP, &status);
    if (OPUS_OK != opus_encoder_ctl(mEncoderCtx, OPUS_SET_COMPLEXITY(OPUS_CODEC_COMPLEXITY)))
        ICELogError(<< "Failed to set Opus encoder complexity");
    // Decoder creation is postponed until first packet arriving (because it may use different channel number
}

void OpusCodec::applyParams(const Params &params)
{
    int error;
    if (OPUS_OK != (error = opus_encoder_ctl(mEncoderCtx, OPUS_SET_DTX(params.mUseDtx ? 1 : 0))))
        ICELogError(<< "Failed to (un)set DTX mode in Opus encoder. Error " << opus_strerror(error));

    if (OPUS_OK != (error = opus_encoder_ctl(mEncoderCtx, OPUS_SET_INBAND_FEC(params.mUseInbandFec ? 1 : 0))))
        ICELogError(<< "Failed to (un)set FEC mode in Opus encoder. Error " << opus_strerror(error));

    if (OPUS_OK != (error = opus_encoder_ctl(mEncoderCtx, OPUS_SET_BITRATE(params.mTargetBitrate ? params.mTargetBitrate : OPUS_AUTO))))
        ICELogError(<< "Failed to (un)set target bandwidth. Error " << opus_strerror(error));

    if (OPUS_OK != (error = opus_encoder_ctl(mEncoderCtx, OPUS_SET_PACKET_LOSS_PERC(params.mExpectedPacketLoss))))
        ICELogError(<< "Failed to (un)set expected packet loss. Error " << opus_strerror(error));

    // mDecodeResampler.start(channels(), 48000, mSamplerate);
}

OpusCodec::~OpusCodec()
{
    if (mDecoderCtx)
    {
        opus_decoder_destroy(mDecoderCtx);
        mDecoderCtx = nullptr;
    }

    if (mEncoderCtx)
    {
        opus_encoder_destroy(mEncoderCtx);
        mEncoderCtx = nullptr;
    }
}

Codec::Info OpusCodec::info() {
    return {
        .mName = OPUS_CODEC_NAME,
        .mSamplerate = mSamplerate,
        .mChannels = mChannels,
        .mPcmLength = (int)(mSamplerate / 1000 * sizeof(short) * mChannels * mPTime),
        .mFrameTime = mPTime,
        .mRtpLength = 0 /* VBR */
    };
}

Codec::EncodeResult OpusCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    // Send number of samples for input and number of bytes for output
    int written = opus_encode(mEncoderCtx, (const opus_int16*)input.data(), input.size_bytes() / (sizeof(short) * channels()),
                              output.data(), output.size_bytes() / (sizeof(short) * channels()));
    if (written < 0)
        return {.mEncoded = 0};
    else
        return {.mEncoded = (size_t)written};
}

Codec::DecodeResult OpusCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    int result = 0;

    // Examine the number of channels available in incoming packet
    int nr_of_channels = opus_packet_get_nb_channels(input.data());

    // Recreate decoder if needed
    if (mDecoderChannels != nr_of_channels)
    {
        if (mDecoderCtx)
        {
            opus_decoder_destroy(mDecoderCtx);
            mDecoderCtx = nullptr;
        }
        mDecoderChannels = nr_of_channels;
    }

    if (!mDecoderCtx)
    {
        int status = 0;
        mDecoderCtx = opus_decoder_create(mSamplerate, mDecoderChannels, &status);
        if (status)
            return {0};
    }

    int nr_of_frames = opus_decoder_get_nb_samples(mDecoderCtx, input.data(), input.size_bytes());
    if (nr_of_frames <= 0)
        return {0};

    // We support stereo and mono here.
    int buffer_capacity = nr_of_frames * sizeof(opus_int16) * nr_of_channels;
    opus_int16 *buffer_decode = (opus_int16 *)alloca(buffer_capacity);
    int decoded = opus_decode(mDecoderCtx, input.data(), input.size_bytes(),
                              buffer_decode, nr_of_frames, 0);
    if (decoded < 0)
    {
        ICELogCritical(<< "opus_decode() returned " << decoded);
        return {0};
    }

    opus_int16 *buffer_stereo = nullptr;
    int buffer_stereo_capacity = buffer_capacity * 2;

    switch (nr_of_channels) {
        case 1:
            // Convert to stereo before
            buffer_stereo = (opus_int16 *) alloca(buffer_stereo_capacity);
            for (int i = 0; i < nr_of_frames; i++) {
                buffer_stereo[i * 2 + 1] = buffer_decode[i];
                buffer_stereo[i * 2] = buffer_decode[i];
            }
            assert(buffer_stereo_capacity <= output.size_bytes());
            memcpy(output.data(), buffer_stereo, buffer_stereo_capacity);
            result = buffer_stereo_capacity;
            break;

        case 2:
            assert(buffer_capacity <= output.size_bytes());
            memcpy(output.data(), buffer_decode, buffer_capacity);
            result = buffer_capacity;
            break;

        default:
            assert(0);
    }

    return {.mDecoded = (size_t)result};
}

size_t OpusCodec::plc(int lostPackets, std::span<uint8_t> output)
{
    // Find how much frames do we need to produce and prefill it with silence
    int frames_per_packet = (int)pcmLength() / (sizeof(opus_int16) * channels());
    memset(output.data(), 0, output.size_bytes());

    // Use this pointer as output
    opus_int16* data_output = reinterpret_cast<opus_int16*>(output.data());

    int nr_of_decoded_frames = 0;

    // Buffer for single lost frame
    opus_int16* buffer_plc = (opus_int16*)alloca(frames_per_packet * mDecoderChannels * sizeof(opus_int16));
    for (int i=0; i<lostPackets; i++)
    {
        nr_of_decoded_frames = opus_decode(mDecoderCtx, nullptr, 0, buffer_plc, frames_per_packet, 0);
        assert(nr_of_decoded_frames == frames_per_packet);
        switch (mDecoderChannels)
        {
            case 1:
                // Convert mono to stereo
                for (int i=0; i < nr_of_decoded_frames; i++)
                    data_output[i * 2] = data_output[i * 2 + 1] = buffer_plc[i];
                data_output += frames_per_packet * mChannels;
                break;

            case 2:
                // Just copy data
                memcpy(data_output, buffer_plc, frames_per_packet * sizeof(opus_int16) * mDecoderChannels);
                data_output += frames_per_packet * mChannels;
                break;
        }
    }
    return ((uint8_t*)data_output - output.data());
}

// -------------- ILBC -------------------
#define ILBC_CODEC_NAME "ILBC"

IlbcCodec::IlbcCodec(int packetTime)
    :mPacketTime(packetTime)
{
    WebRtcIlbcfix_EncoderCreate(&mEncoderCtx);
    WebRtcIlbcfix_DecoderCreate(&mDecoderCtx);
    WebRtcIlbcfix_EncoderInit(mEncoderCtx, mPacketTime);
    WebRtcIlbcfix_DecoderInit(mDecoderCtx, mPacketTime);
}

IlbcCodec::~IlbcCodec()
{
    WebRtcIlbcfix_DecoderFree(mDecoderCtx);
    WebRtcIlbcfix_EncoderFree(mEncoderCtx);
}

Codec::Info IlbcCodec::info()
{
    return {
        .mName = ILBC_CODEC_NAME,
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = mPacketTime * 8 * (int)sizeof(short),
        .mFrameTime = mPacketTime,
        .mRtpLength = (mPacketTime == 20) ? 38 : 50
    };
}

Codec::EncodeResult IlbcCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (input.size_bytes() % pcmLength())
        return {};

    // Declare the data input pointer
    short *dataIn = (short *)input.data();

    // Declare the data output pointer
    char *dataOut = (char *)output.data();

    // Find how much RTP frames will be generated
    unsigned int frames = input.size_bytes() / pcmLength();

    // Generate frames
    for (unsigned int i=0; i<frames; i++)
    {
        WebRtcIlbcfix_Encode(mEncoderCtx, dataIn, pcmLength()/2, (WebRtc_Word16*)dataOut);
        dataIn += pcmLength() / 2;
        dataOut += rtpLength();
    }

    return {frames * rtpLength()};
}

Codec::DecodeResult IlbcCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    unsigned frames = input.size_bytes() / rtpLength();

    char* dataIn = (char*)input.data();
    short* dataOut = (short*)output.data();

    for (unsigned i=0; i < frames; ++i)
    {
        WebRtc_Word16 speechType = 0;
        WebRtcIlbcfix_Decode(mDecoderCtx, (WebRtc_Word16*)dataIn, rtpLength(), dataOut, &speechType);
        dataIn += rtpLength();
        dataOut += pcmLength() / 2;
    }

    return {frames * pcmLength()};
}

size_t IlbcCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    return sizeof(short) * WebRtcIlbcfix_DecodePlc(mDecoderCtx, (WebRtc_Word16*)output.data(), lostFrames);
}

// --- IlbcFactory ---
IlbcCodec::IlbcFactory::IlbcFactory(int ptype20ms, int ptype30ms)
    :mPtime(30), mPType20ms(ptype20ms), mPType30ms(ptype30ms)
{
}

const char* IlbcCodec::IlbcFactory::name()
{
    return ILBC_CODEC_NAME;
}

int IlbcCodec::IlbcFactory::samplerate()
{
    return 8000;
}


int IlbcCodec::IlbcFactory::payloadType()
{
    return mPType30ms;
}

PCodec IlbcCodec::IlbcFactory::create()
{
    return PCodec(new IlbcCodec(mPtime));
}

void IlbcCodec::IlbcFactory::create(CodecMap& codecs)
{
    codecs[mPType20ms] = PCodec(create());
    codecs[mPType30ms] = PCodec(create());
}

void IlbcCodec::IlbcFactory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    if (mPtime == 20 || direction == Sdp_Offer)
    {
        resip::Codec ilbc20(name(), mPType20ms, samplerate());
        ilbc20.parameters() = "ptime=20";
        codecs.push_back(ilbc20);
    }

    if (mPtime == 30 || direction == Sdp_Offer)
    {
        resip::Codec ilbc30(name(), mPType30ms, samplerate());
        codecs.push_back(ilbc30);
    }
}

int IlbcCodec::IlbcFactory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    resip::SdpContents::Session::Medium::CodecContainer::const_iterator codecIter;
    int pt = -1, ptime = 0;
    for (codecIter = codecs.begin(); codecIter != codecs.end(); codecIter++)
    {
        if (stricmp(codecIter->getName().c_str(), "ilbc") == 0)
        {
            const resip::Data& p = codecIter->parameters();
            int codecPtime = 0;
            if (p.size())
            {
                if (strstr(p.c_str(), "mode=") == p.c_str())
                    sscanf(p.c_str(), "mode=%d", &codecPtime);
                else
                    if (strstr(p.c_str(), "ptime="))
                        sscanf(p.c_str(), "ptime=%d", &codecPtime);
            }

            if (codecPtime > ptime)
            {
                pt = codecIter->payloadType();
                ptime = codecPtime;
            }
            else
                if (!codecPtime)
                {
                    // Suppose it is 30ms ilbc
                    pt = codecIter->payloadType();
                    ptime = 30;
                }
        }
    }

    if (pt != -1)
        mPtime = ptime;
    return pt;
}

// --- IsacCodec(s) ---
#define ISAC_CODEC_NAME "ISAC"

IsacCodec::IsacCodec(int samplerate)
    :mSamplerate(samplerate)
{
    // This code initializes isac encoder to automatic mode - it will adjust its bitrate automatically.
    // Frame time is 60 ms
    WebRtcIsacfix_Create(&mEncoderCtx);
    WebRtcIsacfix_EncoderInit(mEncoderCtx, 0);
    //WebRtcIsacfix_Control(mEncoderCtx, mSamplerate, 30);
    //WebRtcIsacfix_SetEncSampRate(mEncoderCtx, mSampleRate == 16000 ? kIsacWideband : kIsacSuperWideband);
    WebRtcIsacfix_Create(&mDecoderCtx);
    WebRtcIsacfix_DecoderInit(mDecoderCtx);
    //WebRtcIsacfix_Control(mDecoderCtx, mSamplerate, 30);
    //WebRtcIsacfix_SetDecSampRate(mDecoderCtx, mSampleRate == 16000 ? kIsacWideband : kIsacSuperWideband);
}

IsacCodec::~IsacCodec()
{
    WebRtcIsacfix_Free(mEncoderCtx); mEncoderCtx = NULL;
    WebRtcIsacfix_Free(mDecoderCtx); mDecoderCtx = NULL;
}

Codec::Info IsacCodec::info() {
    return {
        .mName = "isac",
        .mSamplerate = mSamplerate,
        .mChannels = 1,
        .mPcmLength = 60 * mSamplerate / 1000 * 2,
        .mFrameTime = 60,
        .mRtpLength = 0
    };
}

Codec::EncodeResult IsacCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    unsigned nrOfSamples = input.size_bytes() / 2;
    unsigned timeLength = nrOfSamples / (mSamplerate / 1000);
    int encoded = 0;
    char* dataOut = (char*)output.data();
    const WebRtc_Word16* dataIn = (const WebRtc_Word16*)input.data();

    // Iterate 10 milliseconds chunks
    for (unsigned i=0; i<timeLength/10; i++)
    {
        encoded = WebRtcIsacfix_Encode(mEncoderCtx, dataIn + samplerate() / 100 * i, (WebRtc_Word16*)dataOut);
        if (encoded > 0)
            dataOut += encoded;
    }
    return {.mEncoded = (size_t)(dataOut - (char*)output.data())};
}

Codec::DecodeResult IsacCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    WebRtc_Word16 speechType = 0;
    unsigned produced = WebRtcIsacfix_Decode(mDecoderCtx, (const WebRtc_UWord16*)input.data(), input.size_bytes(), (WebRtc_Word16*)output.data(), &speechType);
    if (produced == (unsigned)-1)
        return {.mDecoded = 0};

    return {.mDecoded = produced * 2};
}

size_t IsacCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    // lostFrames are 30-milliseconds frames; but used encoding mode is 60 milliseconds.
    // So lostFrames * 2
    lostFrames *=2 ;
    if (-1 == WebRtcIsacfix_DecodePlc(mDecoderCtx, (WebRtc_Word16*)output.data(), lostFrames ))
        return 0;

    return lostFrames * 30 * (samplerate()/1000 * sizeof(short));
}

// --- IsacFactory16K ---
IsacCodec::IsacFactory16K::IsacFactory16K(int ptype)
    :mPType(ptype)
{}

const char* IsacCodec::IsacFactory16K::name()
{
    return ISAC_CODEC_NAME;
}

int IsacCodec::IsacFactory16K::samplerate()
{
    return 16000;
}

int IsacCodec::IsacFactory16K::payloadType()
{
    return mPType;
}

PCodec IsacCodec::IsacFactory16K::create()
{
    return PCodec(new IsacCodec(16000));
}

// --- IsacFactory32K ---
IsacCodec::IsacFactory32K::IsacFactory32K(int ptype)
    :mPType(ptype)
{}


const char* IsacCodec::IsacFactory32K::name()
{
    return ISAC_CODEC_NAME;
}

int IsacCodec::IsacFactory32K::samplerate()
{
    return 32000;
}

int IsacCodec::IsacFactory32K::payloadType()
{
    return mPType;
}

PCodec IsacCodec::IsacFactory32K::create()
{
    return PCodec(new IsacCodec(32000));
}

// --- G711 ---
#define ULAW_CODEC_NAME "PCMU"
#define ALAW_CODEC_NAME "PCMA"

G711Codec::G711Codec(int type)
    :mType(type)
{}

G711Codec::~G711Codec()
{}

Codec::Info G711Codec::info() {
    return {
        .mName = mType == ALaw ? "PCMA" : "PCMU",
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 10 * 16,
        .mFrameTime = 10,
        .mRtpLength = 10 * 8
    };
}

Codec::EncodeResult G711Codec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    int result;
    if (mType == ALaw)
        result = WebRtcG711_EncodeA(nullptr, (WebRtc_Word16*)input.data(), input.size_bytes() / 2, (WebRtc_Word16*)output.data());
    else
        result = WebRtcG711_EncodeU(nullptr, (WebRtc_Word16*)input.data(), input.size_bytes() / 2, (WebRtc_Word16*)output.data());

    if (result < 0)
        return {.mEncoded = 0};

    return {.mEncoded = (size_t) result};
}

Codec::DecodeResult G711Codec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    assert(output.size_bytes() >= input.size_bytes() * 2);

    int result;
    WebRtc_Word16 speechType;

    if (mType == ALaw)
        result = WebRtcG711_DecodeA(nullptr, (WebRtc_Word16*)input.data(), input.size_bytes(), (WebRtc_Word16*)output.data(), &speechType);
    else
        result = WebRtcG711_DecodeU(nullptr, (WebRtc_Word16*)input.data(), input.size_bytes(), (WebRtc_Word16*)output.data(), &speechType);

    if (result < 0)
        return {.mDecoded = 0};

    return {.mDecoded = (size_t)result * 2};
}

size_t G711Codec::plc(int lostSamples, std::span<uint8_t> output)
{
    return 0;
}

// --- AlawFactory ---
const char* G711Codec::AlawFactory::name()
{
    return ALAW_CODEC_NAME;
}
int G711Codec::AlawFactory::samplerate()
{
    return 8000;
}
int G711Codec::AlawFactory::payloadType()
{
    return 8;
}
PCodec G711Codec::AlawFactory::create()
{
    return PCodec(new G711Codec(G711Codec::ALaw));
}

// --- UlawFactory ---
const char* G711Codec::UlawFactory::name()
{
    return ULAW_CODEC_NAME;
}
int G711Codec::UlawFactory::samplerate()
{
    return 8000;
}
int G711Codec::UlawFactory::payloadType()
{
    return 0;
}
PCodec G711Codec::UlawFactory::create()
{
    return PCodec(new G711Codec(G711Codec::ULaw));
}

// ---------- GsmCodec -------------

GsmCodec::GsmFactory::GsmFactory(Type codecType, int pt)
    :mCodecType(codecType), mPayloadType(pt)
{}

const char* GsmCodec::GsmFactory::name()
{
    return GSM_MIME_NAME;
}

int GsmCodec::GsmFactory::samplerate()
{
    return 8000;
}

int GsmCodec::GsmFactory::payloadType()
{
    return mPayloadType;
}

PCodec GsmCodec::GsmFactory::create()
{
    return PCodec(new GsmCodec(mCodecType));
}

GsmCodec::GsmCodec(Type codecType)
    :mCodecType(codecType)
{
    mGSM = gsm_create();
    if (codecType != Type::Bytes_33)
    {
        int optval = 1;
        gsm_option(mGSM, GSM_OPT_WAV49, &optval);
    }
}

GsmCodec::~GsmCodec()
{
    gsm_destroy(mGSM); mGSM = nullptr;
}

Codec::Info GsmCodec::info() {
    int rtpLength = 0;
    switch (mCodecType)
    {
    case Type::Bytes_31: rtpLength = GSM_RTPFRAME_SIZE_31; break;
    case Type::Bytes_32: rtpLength = GSM_RTPFRAME_SIZE_32; break;
    case Type::Bytes_33: rtpLength = GSM_RTPFRAME_SIZE_33; break;
    case Type::Bytes_65: rtpLength = GSM_RTPFRAME_SIZE_32 + GSM_RTPFRAME_SIZE_33; break;
    default:             rtpLength = GSM_RTPFRAME_SIZE_33;
    }

    return {
        .mName = "GSM-06.10",
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = GSM_AUDIOFRAME_TIME * 16,
        .mFrameTime = GSM_AUDIOFRAME_TIME,
        .mRtpLength = rtpLength
    };
}

Codec::EncodeResult GsmCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    int outputBytes = 0;
    char* outputBuffer = (char*)output.data();

    for (int i = 0; i < input.size_bytes() / pcmLength(); i++)
    {
        gsm_encode(mGSM, (gsm_signal *)input.data()+160*i, (gsm_byte*)outputBuffer);
        outputBuffer += rtpLength();
        outputBytes += rtpLength();
    }
    return {.mEncoded = (size_t)outputBytes};
}


Codec::DecodeResult GsmCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (input.size_bytes() % rtpLength() != 0)
        return {.mDecoded = 0};

    int i=0;
    for (i = 0; i < input.size_bytes() / rtpLength(); i++)
        gsm_decode(mGSM, (gsm_byte *)input.data() + 33 * i, (gsm_signal *)output.data() + 160 * i);

    return {.mDecoded = (size_t)i * 320};
}

size_t GsmCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    if (output.size_bytes() < lostFrames * pcmLength())
        return 0;

    // Return silence frames
    memset(output.data(), 0, lostFrames * pcmLength());
    return lostFrames * pcmLength();
}


// ------------- G722Codec -----------------

G722Codec::G722Codec()
{
    mEncoder = g722_encode_init(nullptr, 64000, 0);
    mDecoder = g722_decode_init(nullptr, 64000, 0);

}

G722Codec::~G722Codec()
{
    g722_decode_release((g722_decode_state_t*)mDecoder); mDecoder = nullptr;
    g722_encode_release((g722_encode_state_t*)mEncoder); mEncoder = nullptr;
}

Codec::Info G722Codec::info() {
    // ToDo: double check the G722 calls - remember RFC has bug about samplerate
    return {
        .mName = G722_MIME_NAME,
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 640,
        .mFrameTime = 20,
        .mRtpLength = 160
    };
}

Codec::EncodeResult G722Codec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (output.size_bytes() < input.size_bytes() / 4)
        return {.mEncoded = 0}; // Destination buffer not big enough

    int r = g722_encode((g722_encode_state_t *)mEncoder, (unsigned char*)output.data(), ( short*)input.data(), input.size_bytes() / 2);
    if (r < 0)
        return {.mEncoded = 0};

    return {.mEncoded = (size_t)r};
}

Codec::DecodeResult G722Codec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (output.size_bytes() < input.size_bytes() * 4)
        return {.mDecoded = 0}; // Destination buffer not big enough

    int r = g722_decode((g722_decode_state_t *)mDecoder, (short*)output.data(), (unsigned char*)input.data(), input.size_bytes()) * 2;
    if (r < 0)
        return {.mDecoded = 0};
    return {.mDecoded = (size_t)r};
}

size_t G722Codec::plc(int lostFrames, std::span<uint8_t> output)
{
    if (output.size_bytes() < lostFrames * pcmLength())
        return 0;

    // Return silence frames
    memset(output.data(), 0, lostFrames * pcmLength());
    return lostFrames * pcmLength();
}

G722Codec::G722Factory::G722Factory()
{}

const char* G722Codec::G722Factory::name()
{
    return G722_MIME_NAME;
}

int G722Codec::G722Factory::samplerate()
{
    // Although G722 uses 16000 as rate for timestamping RTP frames - in fact it is 8KHz codec. So return 8KHz here.
    return 8000;
}

int G722Codec::G722Factory::payloadType()
{
    return G722_PAYLOAD_TYPE;
}

PCodec G722Codec::G722Factory::create()
{
    return PCodec(new G722Codec());
}

#ifndef TARGET_ANDROID
// --------------- GsmHrCodec -------------------

// Returns false if error occured.
#define GSMHR_GOODSPEECH  (0)
#define GSMHR_GOODSID     (2)
#define GSMHR_NODATA      (7)

static bool repackHalfRate(BitReader& br, uint16_t frame[22], bool& lastItem)
{
    // Check if it is last chunk
    lastItem = br.readBit() == 0;

    // Read frame type
    uint8_t frametype = (uint8_t)br.readBits(3); // Read frame type

    // Read reserved bits
    br.readBits(4);

    switch (frametype)
    {
    case GSMHR_GOODSPEECH:
    case GSMHR_GOODSID:
        frame[0] = br.readBits(5); // R0
        frame[1] = br.readBits(11); // LPC1
        frame[2] = br.readBits(9); // LPC2
        frame[3] = br.readBits(8); // LPC3
        frame[4] = br.readBits(1); // INT_LPC
        frame[5] = br.readBits(2); // MODE
        // Parse speech frame
        if (frame[5])
        {
            // Voiced mode
            frame[6] = br.readBits(8); // LAG_1
            frame[7] = br.readBits(9); // CODE_1
            frame[8] = br.readBits(5); // GSP0_1

            frame[9] = br.readBits(4); // LAG_2
            frame[10] = br.readBits(9); // CODE_2
            frame[11] = br.readBits(5); // GSP0_2

            frame[12] = br.readBits(4); // LAG_3
            frame[13] = br.readBits(9); // CODE_3
            frame[14] = br.readBits(5); // GSP0_3

            frame[15] = br.readBits(4); // LAG_4
            frame[16] = br.readBits(9); // CODE_4
            frame[17] = br.readBits(5); // GSP0_4
        }
        else
        {
            // Unvoiced mode
            frame[6] = br.readBits(7); // CODE1_1
            frame[7] = br.readBits(7); // CODE2_1
            frame[8] = br.readBits(5); // GSP0_1

            frame[9] = br.readBits(7); // CODE1_2
            frame[10] = br.readBits(7); // CODE2_2
            frame[11] = br.readBits(5); // GSP0_2

            frame[12] = br.readBits(7); // CODE1_3
            frame[13] = br.readBits(7); // CODE2_3
            frame[14] = br.readBits(5); // GSP0_3;

            frame[15] = br.readBits(7); // CODE1_4;
            frame[16] = br.readBits(7); // CODE2_4;
            frame[17] = br.readBits(5); // GSP0_4;
        }

        frame[18] = 0; frame[19] = 0;
        frame[20] = 0; frame[21] = 0;
        break;

    case GSMHR_NODATA:
        break;

    default:
        return false;
    }

    return true;
}

GsmHrCodec::GsmHrCodec()
{
    mDecoder = new GsmHr::Codec();
}

GsmHrCodec::~GsmHrCodec()
{
    delete reinterpret_cast<GsmHr::Codec*>(mDecoder);
    mDecoder = nullptr;
}

Codec::Info GsmHrCodec::info() {
    return {
        .mName = "GSM-HR-08",
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 20 * 8 * 2,
        .mFrameTime = 20,
        .mRtpLength = 0
    };
}

Codec::EncodeResult GsmHrCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    // Not supported yet
    return {.mEncoded = 0};
}

static const int params_unvoiced[] = {
    5,	/* R0 */
    11,	/* k1Tok3 */
    9,	/* k4Tok6 */
    8,	/* k7Tok10 */
    1,	/* softInterpolation */
    2,	/* voicingDecision */
    7,	/* code1_1 */
    7,	/* code2_1 */
    5,	/* gsp0_1 */
    7,	/* code1_2 */
    7,	/* code2_2 */
    5,	/* gsp0_2 */
    7,	/* code1_3 */
    7,	/* code2_3 */
    5,	/* gsp0_3 */
    7,	/* code1_4 */
    7,	/* code2_4 */
    5,	/* gsp0_4 */
};

static const int params_voiced[] = {
    5,	/* R0 */
    11,	/* k1Tok3 */
    9,	/* k4Tok6 */
    8,	/* k7Tok10 */
    1,	/* softInterpolation */
    2,	/* voicingDecision */
    8,	/* frameLag */
    9,	/* code_1 */
    5,	/* gsp0_1 */
    4,	/* deltaLag_2 */
    9,	/* code_2 */
    5,	/* gsp0_2 */
    4,	/* deltaLag_3 */
    9,	/* code_3 */
    5,	/* gsp0_3 */
    4,	/* deltaLag_4 */
    9,	/* code_4 */
    5,	/* gsp0_4 */
};

static int
msb_get_bit(const uint8_t *buf, int bn)
{
    int pos_byte = bn >> 3;
    int pos_bit  = 7 - (bn & 7);

    return (buf[pos_byte] >> pos_bit) & 1;
}

static int
hr_ref_from_canon(uint16_t *hr_ref, const uint8_t *canon)
{
    int i, j, voiced;
    const int *params;

    voiced = (msb_get_bit(canon, 34) << 1) | msb_get_bit(canon, 35);
    params = voiced ? &params_voiced[0] : &params_unvoiced[0];

    for (i=0,j=0; i<18; i++)
    {
        uint16_t w;
        int l, k;

        l = params[i];

        w = 0;
        for (k=0; k<l; k++)
            w = (w << 1) | msb_get_bit(canon, j+k);
        hr_ref[i] = w;

        j += l;
    }

    return 0;
}


/*
 * Log from gapk:
 *  [+] PQ: Adding file input (blk_len=15)
[+] PQ: Adding conversion from rtp-hr-ietf to canon (for codec hr)
[+] PQ: Adding conversion from canon to hr-ref-dec (for codec hr)
[+] PQ: Adding Codec hr, decoding from format hr-ref-dec
[+] PQ: Adding conversion from canon to rawpcm-s16le (for codec pcm)
[+] PQ: Adding file output (blk_len=320)
*/
Codec::DecodeResult GsmHrCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    ByteBuffer bb(input, ByteBuffer::CopyBehavior::UseExternal);
    BitReader br(bb);
    uint16_t hr_ref[22];

    hr_ref_from_canon(hr_ref, input.data() + 1);
    hr_ref[18] = 0;		/* BFI : 1 bit */
    hr_ref[19] = 0;		/* UFI : 1 bit */
    hr_ref[20] = 0;		/* SID : 2 bit */
    hr_ref[21] = 0;		/* TAF : 1 bit */

    reinterpret_cast<GsmHr::Codec*>(mDecoder)->speechDecoder((int16_t*)hr_ref, (int16_t*)output.data());
    return {.mDecoded = 320};
}

size_t GsmHrCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    return 0;
}

GsmHrCodec::GsmHrFactory::GsmHrFactory(int ptype)
    :mPtype(ptype)
{}

const char* GsmHrCodec::GsmHrFactory::name()
{
    return "GSM-HR-08";
}

int GsmHrCodec::GsmHrFactory::samplerate()
{
    return 8000;
}

int GsmHrCodec::GsmHrFactory::payloadType()
{
    return mPtype;
}

PCodec GsmHrCodec::GsmHrFactory::create()
{
    return PCodec(new GsmHrCodec());
}
#endif
