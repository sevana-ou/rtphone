// Avoid linking issues on embedded systems
#if !defined(TARGET_ANDROID) && !defined(TARGET_OPENWRT) && !defined(TARGET_RPI)

#include "MT_AmrCodec.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_IuUP.h"
#include "../helper/HL_Log.h"

#define LOG_SUBSYSTEM "media"
using namespace MT;


// Constant of AMR-NB frame lengths in bytes.
const uint8_t amrnb_framelen[9] =
    {12,  13,  15,  17,  19,  20,  26,  31,  5};

const uint16_t amrnb_framelenbits[9] =
    {95, 103, 118, 134, 148, 159, 204, 244, 39};


// Constant of AMR-WB frame lengths in bytes.
const uint8_t  amrwb_framelen[10] =
    {17,   23,  32,  37,  40,  46,  50,  58,  60,  5 /* SID packet */};
const uint16_t amrwb_framelenbits[10] =
    {132, 177, 253, 285, 317, 365, 397, 461, 477, 40 /* SID packet */};

// Helper routines

struct AmrPayloadInfo
{
    const uint8_t*  mPayload = nullptr;
    int             mPayloadLength = 0;
    bool            mOctetAligned = false;
    bool            mInterleaving = false;
    bool            mWideband = false;
    uint64_t        mCurrentTimestamp = 0;
};


struct AmrFrame
{
    uint8_t               mFrameType = 0;
    uint8_t               mMode = 0;
    bool                  mGoodQuality = false;
    uint64_t              mTimestamp = 0;
    std::shared_ptr<ByteBuffer> mData;
    uint8_t               mSTI = 0;
};

struct AmrPayload
{
    uint8_t               mCodeModeRequest = 0;
    std::vector<AmrFrame> mFrames;
    bool                  mDiscardPacket = false;
};

// ARM RTP payload has next structure
//   Header
//   Table of Contents
//   Frames
static AmrPayload parseAmrPayload(AmrPayloadInfo& input, size_t& cngCounter)
{
    AmrPayload result;

    // Do not skip packet by default; I suppose packet is good enough by default.
    result.mDiscardPacket = false;

    // Wrap incoming data with ByteArray to make bit dequeuing easy
    ByteBuffer byte_reader(input.mPayload, static_cast<size_t>(input.mPayloadLength));
    BitReader  bit_reader (input.mPayload, static_cast<size_t>(input.mPayloadLength));

    // In bandwidth-efficient mode, the payload header simply consists of a
    //   4-bit codec mode request:
    // CMR (4 bits): Indicates a codec mode request sent to the speech
    //      encoder at the site of the receiver of this payload.  The value of
    //      the CMR field is set to the frame type index of the corresponding
    //      speech mode being requested.  The frame type index may be 0-7 for
    //      AMR, as defined in Table 1a in [2], or 0-8 for AMR-WB, as defined
    //      in Table 1a in [4].  CMR value 15 indicates that no mode request
    //      is present, and other values are for future use.
    result.mCodeModeRequest = static_cast<uint8_t>(bit_reader.readBits(4));

    // Consume extra 4 bits for octet aligned profile
    if (input.mOctetAligned)
        bit_reader.readBits(4);

    // Skip interleaving flags for now for octet aligned mode
    if (input.mInterleaving && input.mOctetAligned)
        bit_reader.readBits(8);

    // Silence codec mode constant (it differs for wideband and narrowband codecs)
    uint8_t SID_FT = input.mWideband ? 9 : 8;

    // Table of contents
    uint8_t F, FT, Q;

    do
    {
        // Read TOC. It is still relates to RTP part of AMR frames packing; not the AMR frame itself.
        //    F (1 bit): If set to 1, indicates that this frame is followed by
        // another speech frame in this payload; if set to 0, indicates that
        // this frame is the last frame in this payload.
        F = bit_reader.readBit();

        //    FT (4 bits): Frame type index, indicating either the AMR or AMR-WB
        // speech coding mode or comfort noise (SID) mode of the
        // corresponding frame carried in this payload.
        FT = static_cast<uint8_t>(bit_reader.readBits(4));

        //   Q (1 bit): Frame quality indicator.  If set to 0, indicates the
        // corresponding frame is severely damaged, and the receiver should
        // set the RX_TYPE (see [6]) to either SPEECH_BAD or SID_BAD
        // depending on the frame type (FT).
        Q = bit_reader.readBit();

        // Handle padding for octet alignment
        if (input.mOctetAligned)
            bit_reader.readBits(2);

        AmrFrame frame;
        frame.mFrameType = FT;
        frame.mSTI = 0;
        frame.mMode = FT < SID_FT ? FT : 0xFF;
        frame.mGoodQuality = Q == 1;
        frame.mTimestamp = input.mCurrentTimestamp;
        result.mFrames.push_back(frame);
        input.mCurrentTimestamp += input.mWideband ? 320 : 160;
        if (FT == SID_FT)
            cngCounter++;
    }
    while (F != 0);

    for (size_t frameIndex=0; frameIndex < result.mFrames.size() && !result.mDiscardPacket; frameIndex++)
    {
        AmrFrame& f = result.mFrames[frameIndex];

        // If receiving a ToC entry with a FT value in the range 9-14 for AMR or
        //   10-13 for AMR-WB, the whole packet SHOULD be discarded.  This is to
        //   avoid the loss of data synchronization in the depacketization
        //   process, which can result in a huge degradation in speech quality.
        bool discard = input.mWideband ? (f.mFrameType >= 10 && f.mFrameType <= 13) : (f.mFrameType >= 9 && f.mFrameType <= 14);
        if (discard)
        {
            result.mDiscardPacket = true;
            continue;
        }

        // if (input.mWideband && f.mMode == 0xFF /* CNG */)
        // {
        //     int a = 1;
        // }`

        if (input.mWideband && f.mFrameType == 15)
        {
            // DTX, no sense to decode the data
            continue;
        }

        if (input.mWideband && f.mFrameType == 14)
        {
            // Speech lost code only
            continue;
        }

        if (!f.mGoodQuality)
        {
            // Bad quality, frame is damaged
            continue;
        }

        size_t bitsLength = input.mWideband ? amrwb_framelenbits[f.mFrameType]  : amrnb_framelenbits[f.mFrameType];
        size_t byteLength = input.mWideband ? amrwb_framelen[f.mFrameType]      : amrnb_framelen[f.mFrameType];

        if (bitsLength > 0)
        {
            if (input.mOctetAligned)
            {
                if (byte_reader.size() < byteLength)
                    f.mGoodQuality = false;
                else
                {
                    // It is octet aligned scheme, so we are on byte boundary now
                    size_t byteOffset = bit_reader.position() / 8;

                    // Copy data of AMR frame
                    if (byteOffset + byteLength <= input.mPayloadLength)
                    {
                        f.mData = std::make_shared<ByteBuffer>();
                        f.mData->resize(byteLength + 1); // payload + header
                        memcpy(f.mData->mutableData() + 1, input.mPayload + byteOffset, byteLength);

                        // Add header for decoder
                        f.mData->mutableData()[0] = (f.mFrameType << 3) | (1 << 2);
                    }
                    else
                    {
                        ICELogError(<< "Problem parsing AMR header: octet-aligned is set, available " << int(input.mPayloadLength - byteOffset)
                                    << " bytes but requested " << (int)byteLength);
                        result.mDiscardPacket = true;
                        continue;
                    }
                }
            }
            else
            {
                // Allocate place for copying
                f.mData = std::make_shared<ByteBuffer>();
                f.mData->resize(bitsLength / 8 + ((bitsLength % 8) ? 1 : 0) + 1);

                // Add header for decoder
                f.mData->mutableData()[0] = (f.mFrameType << 3) | (1 << 2);

                // Read bits
                if (bit_reader.readBits(f.mData->mutableData() + 1, bitsLength /*+ bitsLength*/ ) < (size_t)bitsLength)
                    f.mGoodQuality = false;
            }
        }
    }

    // Padding bits are skipped
    return result;
}

AmrNbCodec::CodecFactory::CodecFactory(const AmrCodecConfig& config)
    :mConfig(config)
{

}

const char* AmrNbCodec::CodecFactory::name()
{
    return MT_AMRNB_CODECNAME;
}

int AmrNbCodec::CodecFactory::samplerate()
{
    return 8000;
}

int AmrNbCodec::CodecFactory::payloadType()
{
    return mConfig.mPayloadType;
}


void AmrNbCodec::CodecFactory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{}

int AmrNbCodec::CodecFactory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    return 0;
}

void AmrNbCodec::CodecFactory::create(CodecMap& codecs)
{
    codecs[payloadType()] = std::shared_ptr<Codec>(new AmrNbCodec(mConfig));
}

PCodec AmrNbCodec::CodecFactory::create()
{
    return PCodec(new AmrNbCodec(mConfig));
}


AmrNbCodec::AmrNbCodec(const AmrCodecConfig& config)
    :mConfig(config)
{
    // Contexts are created lazily (see ensureEncoder/ensureDecoder) - a codec
    // resolved only for network-MOS metadata never allocates them.
}

void AmrNbCodec::ensureEncoder()
{
    if (!mEncoderCtx)
        mEncoderCtx = Encoder_Interface_init(1);
}

void AmrNbCodec::ensureDecoder()
{
    if (!mDecoderCtx)
        mDecoderCtx = Decoder_Interface_init();
}

AmrNbCodec::~AmrNbCodec()
{
    if (mEncoderCtx)
    {
        Encoder_Interface_exit(mEncoderCtx);
        mEncoderCtx = nullptr;
    }

    if (mDecoderCtx)
    {
        Decoder_Interface_exit(mDecoderCtx);
        mDecoderCtx = nullptr;
    }
}

Codec::Info AmrNbCodec::info()
{
    return {
        .mName = MT_AMRNB_CODECNAME,
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 20 * 16,
        .mFrameTime = 20,
        .mRtpLength = 0
    };
}

Codec::EncodeResult AmrNbCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    ensureEncoder();

    if (input.size_bytes() % pcmLength())
        return {.mEncoded = 0};

    // Declare the data input pointer
    auto *dataIn = (const short *)input.data();

    // Declare the data output pointer
    auto *dataOut = (unsigned char *)output.data();

    // Find how much RTP frames will be generated
    unsigned int frames = input.size_bytes() / pcmLength();

    // Generate frames
    for (unsigned int i = 0; i < frames; i++)
    {
        dataOut += Encoder_Interface_Encode(mEncoderCtx, Mode::MRDTX, dataIn, dataOut, 1);
        dataIn += pcmLength() / 2;
    }

    return {.mEncoded = (size_t)(dataOut - (unsigned char*)output.data())};
}

#define L_FRAME 160
#define AMR_BITRATE_DTX 15
Codec::DecodeResult AmrNbCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    ensureDecoder();

    if (mConfig.mOctetAligned)
        return {.mDecoded = 0};

    if (mConfig.mIuUP)
    {
        // Try to parse IuUP frame
        IuUP::Frame frame;
        if (!IuUP::parse2((const uint8_t*)input.data(), input.size_bytes(), frame))
            return {0};

        // Check if CRC failed - it is check from IuUP data
        if (!frame.mHeaderCrcOk || !frame.mPayloadCrcOk)
        {
            ICELogInfo(<< "CRC check failed.");
            return {0};
        }

        // Build NB frame to decode
        ByteBuffer dataToDecode;
        dataToDecode.resize(1 + frame.mPayloadSize); // Reserve place

        // Copy AMR data
        memmove(dataToDecode.mutableData() + 1, frame.mPayload, frame.mPayloadSize);

        uint8_t frameType = 0xFF;
        for (uint8_t ftIndex = 0; ftIndex <= 9 && frameType == 0xFF; ftIndex++)
            if (amrnb_framelen[ftIndex] == frame.mPayloadSize)
                frameType = ftIndex;

        // Check if frameType comparing is correct
        if (frameType == 0xFF)
            return {0};

        dataToDecode.mutableData()[0] = (frameType << 3) | (1 << 2);

        Decoder_Interface_Decode(mDecoderCtx, (const unsigned char*)dataToDecode.data(), (short*)output.data(), 0);
        return {.mDecoded = (size_t)pcmLength()};
    }
    else
    {
        if (output.size_bytes() < pcmLength())
            return {.mDecoded = 0};

        if (input.size_bytes() == 0)
        { // PLC part
            unsigned char buffer[32];
            buffer[0] = (AMR_BITRATE_DTX << 3)|4;
            Decoder_Interface_Decode(mDecoderCtx, buffer, (short*)output.data(), 0); // Handle missing data
            return {.mDecoded = (size_t)pcmLength()};
        }

        AmrPayloadInfo info;
        info.mCurrentTimestamp = mCurrentDecoderTimestamp;
        info.mOctetAligned = mConfig.mOctetAligned;
        info.mPayload = input.data();
        info.mPayloadLength = input.size_bytes();
        info.mWideband = false;
        info.mInterleaving = false;

        AmrPayload ap;
        try
        {
            ap = parseAmrPayload(info, mCngCounter);
        }
        catch(...)
        {
            ICELogDebug(<< "Failed to decode AMR payload.");
            return {.mDecoded = 0};
        }
        // Save current timestamp
        mCurrentDecoderTimestamp = info.mCurrentTimestamp;

        // Check if packet is corrupted
        if (ap.mDiscardPacket)
            return {.mDecoded = 0};


        // Check for output buffer capacity
        if (output.size_bytes() < (int)ap.mFrames.size() * pcmLength())
            return {.mDecoded = 0};

        if (ap.mFrames.empty())
        {
            ICELogError(<< "No AMR frames");
        }
        short* dataOut = (short*)output.data();
        for (AmrFrame& frame: ap.mFrames)
        {
            if (frame.mData)
            {
                // Call decoder
                Decoder_Interface_Decode(mDecoderCtx, (const unsigned char*)frame.mData->data(), (short*)dataOut, 0);
                dataOut += pcmLength() / 2;
            }
        }
        return {.mDecoded = pcmLength() * ap.mFrames.size()};
    }

    return {.mDecoded = (size_t)pcmLength()};
}

size_t AmrNbCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    ensureDecoder();

    if (output.size_bytes() < lostFrames * pcmLength())
        return 0;

    short* dataOut = (short*)output.data();

    for (int i=0; i < lostFrames; i++)
    {
        uint8_t buffer[32];
        buffer[0] = (AMR_BITRATE_DTX << 3)|4;
        Decoder_Interface_Decode(mDecoderCtx, buffer, dataOut, 0); // Handle missing data
        dataOut += L_FRAME;
    }

    return lostFrames * pcmLength();
}

int AmrNbCodec::getSwitchCounter() const
{
    return mSwitchCounter;
}

int AmrNbCodec::getCngCounter() const
{
    return mCngCounter;
}

// -------- AMR WB codec
AmrWbCodec::CodecFactory::CodecFactory(const AmrCodecConfig& config)
    :mConfig(config)
{}

const char* AmrWbCodec::CodecFactory::name()
{
    return MT_AMRWB_CODECNAME;
}

int AmrWbCodec::CodecFactory::samplerate()
{
    return 16000;
}

int AmrWbCodec::CodecFactory::payloadType()
{
    return mConfig.mPayloadType;
}

void AmrWbCodec::CodecFactory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{}

int AmrWbCodec::CodecFactory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    return 0;
}

void AmrWbCodec::CodecFactory::create(CodecMap& codecs)
{
    codecs[payloadType()] = std::shared_ptr<Codec>(new AmrWbCodec(mConfig));
}

PCodec AmrWbCodec::CodecFactory::create()
{
    return PCodec(new AmrWbCodec(mConfig));
}

AmrWbStatistics MT::GAmrWbStatistics;

AmrWbCodec::AmrWbCodec(const AmrCodecConfig& config)
    :mConfig(config)
{
    // Decoder context is created lazily (see ensureDecoder) - a codec resolved
    // only for network-MOS metadata never allocates the AMR-WB decoder state.
}

void AmrWbCodec::ensureDecoder()
{
    if (!mDecoderCtx)
        mDecoderCtx = D_IF_init();
}

AmrWbCodec::~AmrWbCodec()
{
    if (mEncoderCtx)
    {
        //E_IF_exit(mEncoderCtx);
        mEncoderCtx = nullptr;
    }

    if (mDecoderCtx)
    {
        D_IF_exit(mDecoderCtx);
        mDecoderCtx = nullptr;
    }
}

Codec::Info AmrWbCodec::info() {
    return {
        .mName = MT_AMRWB_CODECNAME,
        .mSamplerate = 16000,
        .mChannels = 1,
        .mPcmLength = 20 * 16 * 2,
        .mFrameTime = 20,
        .mRtpLength = 0   /* There is complex structure inside AMR packet which may include multilple frames with various length. */
    };
}


Codec::EncodeResult AmrWbCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    // Still no support for encoding - emit silence instead
    return {.mEncoded = 0};
}

#define L_FRAME 160
#define AMR_BITRATE_DTX 15

Codec::DecodeResult AmrWbCodec::decodeIuup(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    IuUP::Frame frame;
    if (!IuUP::parse2(input.data(), input.size(), frame))
        return {.mDecoded = 0};

    if (!frame.mHeaderCrcOk || !frame.mPayloadCrcOk)
    {
        ICELogInfo(<< "CRC check failed.");
        return {.mDecoded = 0};
    }

    // Reserve space
    ByteBuffer dataToDecode;
    dataToDecode.resize(1 + frame.mPayloadSize);

    // Copy AMR data
    memmove(dataToDecode.mutableData() + 1, frame.mPayload, frame.mPayloadSize);
    uint8_t frameType = 0xFF;
    for (uint8_t ftIndex = 0; ftIndex <= 9 && frameType == 0xFF; ftIndex++)
        if (amrwb_framelen[ftIndex] == frame.mPayloadSize)
            frameType = ftIndex;

    if (frameType == 0xFF)
        return {.mDecoded = 0, .mIsCng = true};

    dataToDecode.mutableData()[0] = (frameType << 3) | (1 << 2);

    D_IF_decode(mDecoderCtx, (const unsigned char*)dataToDecode.data(), (short*)output.data(), 0);
    return {.mDecoded = (size_t)pcmLength()};
}

Codec::DecodeResult AmrWbCodec::decodePlain(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    AmrPayloadInfo info;
    info.mCurrentTimestamp  = mCurrentDecoderTimestamp;
    info.mOctetAligned      = mConfig.mOctetAligned;
    info.mPayload           = input.data();
    info.mPayloadLength     = input.size();
    info.mWideband          = true;
    info.mInterleaving      = false;

    AmrPayload ap;
    try
    {
        ap = parseAmrPayload(info, mCngCounter);
    }
    catch(...)
    {
        GAmrWbStatistics.mNonParsed++;
        ICELogDebug(<< "Failed to decode AMR payload");
        return {.mDecoded = 0};
    }
    // Save current timestamp
    mCurrentDecoderTimestamp = info.mCurrentTimestamp;

    // Check if packet is corrupted
    if (ap.mDiscardPacket)
    {
        GAmrWbStatistics.mDiscarded++;
        return {.mDecoded = 0};
    }

    // Find the required output capacity
    size_t capacity = 0;
    for (AmrFrame& frame: ap.mFrames)
        capacity += frame.mMode == 0xFF /* CNG */ ? pcmLength() : pcmLength();

    if (output.size() < capacity)
        return {.mDecoded = 0};

    short* dataOut = (short*)output.data();
    size_t dataOutSizeInBytes = 0;
    for (AmrFrame& frame: ap.mFrames)
    {
        size_t frameOutputSize = frame.mMode == 0xFF ? pcmLength() : pcmLength();
        memset(dataOut, 0, frameOutputSize);

        if (frame.mData)
        {
            if (frame.mMode == 0xFF)
            {
                // int bp = 1;
            }
            D_IF_decode(mDecoderCtx, (const unsigned char*)frame.mData->data(), (short*)dataOut, 0);
            dataOut += frameOutputSize / 2;
            dataOutSizeInBytes += frameOutputSize;
        }
    }
    return {.mDecoded = dataOutSizeInBytes,
            .mIsCng = ap.mFrames.size() == 1 ? (ap.mFrames.front().mMode == 0xFF) : false};
}

Codec::DecodeResult AmrWbCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    ensureDecoder();

    if (mConfig.mIuUP)
        return decodeIuup(input, output);
    else
        return decodePlain(input, output);
}

size_t AmrWbCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    // ToDo: Check again if PLC works for AMR-WB
    // For now return the silence
    memset(output.data(), 0, output.size_bytes());
    return lostFrames * pcmLength();
    /*
    if (outputCapacity < lostFrames * pcmLength())
        return 0;

    short* dataOut = (short*)output;

    for (int i=0; i < lostFrames; i++)
    {
        unsigned char buffer[32];
        buffer[0] = (AMR_BITRATE_DTX << 3)|4;
        Decoder_Interface_Decode(mDecoderCtx, buffer, dataOut, 0); // Handle missing data
        dataOut += L_FRAME;
    }
    */
}

int AmrWbCodec::getSwitchCounter() const
{
    return mSwitchCounter;
}

int AmrWbCodec::getCngCounter() const
{
    return mCngCounter;
}

// ------------- GSM EFR -----------------

GsmEfrCodec::GsmEfrFactory::GsmEfrFactory(bool iuup, int ptype)
    :mIuUP(iuup), mPayloadType(ptype)
{}

const char* GsmEfrCodec::GsmEfrFactory::name()
{
    return MT_GSMEFR_CODECNAME;
}

int GsmEfrCodec::GsmEfrFactory::samplerate()
{
    return 8000;
}

int GsmEfrCodec::GsmEfrFactory::payloadType()
{
    return mPayloadType;
}

void GsmEfrCodec::GsmEfrFactory::updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{}

int GsmEfrCodec::GsmEfrFactory::processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction)
{
    return 0;
}

void GsmEfrCodec::GsmEfrFactory::create(CodecMap& codecs)
{
    codecs[payloadType()] = std::shared_ptr<Codec>(new GsmEfrCodec(mIuUP));
}

PCodec GsmEfrCodec::GsmEfrFactory::create()
{
    return PCodec(new GsmEfrCodec(mIuUP));
}

GsmEfrCodec::GsmEfrCodec(bool iuup)
    :mIuUP(iuup)
{
    mEncoderCtx = Encoder_Interface_init(1);
    mDecoderCtx = Decoder_Interface_init();
}

GsmEfrCodec::~GsmEfrCodec()
{
    if (mEncoderCtx)
    {
        Encoder_Interface_exit(mEncoderCtx);
        mEncoderCtx = nullptr;
    }

    if (mDecoderCtx)
    {
        Decoder_Interface_exit(mDecoderCtx);
        mDecoderCtx = nullptr;
    }
}

Codec::Info GsmEfrCodec::info()
{
    return {
        .mName = MT_GSMEFR_CODECNAME,
        .mSamplerate = 8000,
        .mChannels = 1,
        .mPcmLength = 20 * 16,
        .mFrameTime = 20,
        .mRtpLength = 0
    };
}

Codec::EncodeResult GsmEfrCodec::encode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (input.size_bytes() % pcmLength())
        return {.mEncoded = 0};

    // Declare the data input pointer
    const short *dataIn = (const short *)input.data();

    // Declare the data output pointer
    unsigned char *dataOut = (unsigned char *)output.data();

    // Find how much RTP frames will be generated
    unsigned int frames = input.size_bytes() / pcmLength();

    // Generate frames
    for (unsigned int i = 0; i < frames; i++)
    {
        dataOut += Encoder_Interface_Encode(mEncoderCtx, Mode::MRDTX, dataIn, dataOut, 1);
        dataIn += pcmLength() / 2;
    }

    return {.mEncoded = frames * rtpLength()};
}

#define L_FRAME 160
#define AMR_BITRATE_DTX 15
#define GSM_EFR_SAMPLES    160
#define GSM_EFR_FRAME_LEN  31

static void
msb_put_bit(uint8_t *buf, int bn, int bit)
{
    int pos_byte = bn >> 3;
    int pos_bit  = 7 - (bn & 7);

    if (bit)
        buf[pos_byte] |=  (1 << pos_bit);
    else
        buf[pos_byte] &= ~(1 << pos_bit);
}

static int
msb_get_bit(const uint8_t *buf, int bn)
{
    int pos_byte = bn >> 3;
    int pos_bit  = 7 - (bn & 7);

    return (buf[pos_byte] >> pos_bit) & 1;
}

const uint16_t gsm690_12_2_bitorder[244] = {
    0,   1,   2,   3,   4,   5,   6,   7,   8,   9,
    10,  11,  12,  13,  14,  23,  15,  16,  17,  18,
    19,  20,  21,  22,  24,  25,  26,  27,  28,  38,
    141,  39, 142,  40, 143,  41, 144,  42, 145,  43,
    146,  44, 147,  45, 148,  46, 149,  47,  97, 150,
    200,  48,  98, 151, 201,  49,  99, 152, 202,  86,
    136, 189, 239,  87, 137, 190, 240,  88, 138, 191,
    241,  91, 194,  92, 195,  93, 196,  94, 197,  95,
    198,  29,  30,  31,  32,  33,  34,  35,  50, 100,
    153, 203,  89, 139, 192, 242,  51, 101, 154, 204,
    55, 105, 158, 208,  90, 140, 193, 243,  59, 109,
    162, 212,  63, 113, 166, 216,  67, 117, 170, 220,
    36,  37,  54,  53,  52,  58,  57,  56,  62,  61,
    60,  66,  65,  64,  70,  69,  68, 104, 103, 102,
    108, 107, 106, 112, 111, 110, 116, 115, 114, 120,
    119, 118, 157, 156, 155, 161, 160, 159, 165, 164,
    163, 169, 168, 167, 173, 172, 171, 207, 206, 205,
    211, 210, 209, 215, 214, 213, 219, 218, 217, 223,
    222, 221,  73,  72,  71,  76,  75,  74,  79,  78,
    77,  82,  81,  80,  85,  84,  83, 123, 122, 121,
    126, 125, 124, 129, 128, 127, 132, 131, 130, 135,
    134, 133, 176, 175, 174, 179, 178, 177, 182, 181,
    180, 185, 184, 183, 188, 187, 186, 226, 225, 224,
    229, 228, 227, 232, 231, 230, 235, 234, 233, 238,
    237, 236,  96, 199,
};

Codec::DecodeResult GsmEfrCodec::decode(std::span<const uint8_t> input, std::span<uint8_t> output)
{
    if (output.size_bytes() < pcmLength())
        return {.mDecoded = 0};

    if (input.size_bytes() == 0)
    { // PLC part
        unsigned char buffer[32];
        buffer[0] = (AMR_BITRATE_DTX << 3)|4;
        Decoder_Interface_Decode(mDecoderCtx, buffer, (short*)output.data(), 0); // Handle missing data
    }
    else
    {
        // Reorder bytes from input to dst
        uint8_t dst[GSM_EFR_FRAME_LEN];
        const uint8_t* src = input.data();
        for (int i=0; i<(GSM_EFR_FRAME_LEN-1); i++)
            dst[i] = (src[i] << 4) | (src[i+1] >> 4);
        dst[GSM_EFR_FRAME_LEN-1] = src[GSM_EFR_FRAME_LEN-1] << 4;

        unsigned char in[GSM_EFR_FRAME_LEN + 1];

        // Reorder bits
        in[0] = 0x3c; /* AMR mode 7 = GSM-EFR, Quality bit is set */
        in[GSM_EFR_FRAME_LEN] = 0x0;

        for (int i=0; i<244; i++)
        {
            int si = gsm690_12_2_bitorder[i];
            int di = i;
            msb_put_bit(in + 1, di, msb_get_bit(dst, si));
        }

        // Decode
        memset(output.data(), 0, pcmLength());
        Decoder_Interface_Decode(mDecoderCtx, in, (short*)output.data(), 0);

        uint8_t* pcm = (uint8_t*)output.data();
        for (int i=0; i<160; i++)
        {
            uint16_t w = ((uint16_t*)output.data())[i];
            pcm[(i<<1)  ] =  w       & 0xff;
            pcm[(i<<1)+1] = (w >> 8) & 0xff;
        }
    }

    return {.mDecoded = (size_t)pcmLength()};
}

size_t GsmEfrCodec::plc(int lostFrames, std::span<uint8_t> output)
{
    if (output.size_bytes() < lostFrames * pcmLength())
        return 0;

    short* dataOut = (short*)output.data();

    for (int i=0; i < lostFrames; i++)
    {
        unsigned char buffer[32];
        buffer[0] = (AMR_BITRATE_DTX << 3)|4;
        Decoder_Interface_Decode(mDecoderCtx, buffer, dataOut, 0); // Handle missing data
        dataOut += L_FRAME;
    }

    return lostFrames * pcmLength();
}

#endif
