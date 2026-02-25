#ifndef MT_AMRCODEC_H
#define MT_AMRCODEC_H

#include "../engine_config.h"
#include <map>
#include <span>

#include "MT_Codec.h"
#include "../helper/HL_Pointer.h"

# include "opencore-amr/amrnb/interf_enc.h"
# include "opencore-amr/amrnb/interf_dec.h"

# include "opencore-amr/amrwb/if_rom.h"
# include "opencore-amr/amrwb/dec_if.h"

namespace MT
{
struct AmrCodecConfig
{
    bool    mIuUP = false;
    bool    mOctetAligned = false;
    int     mPayloadType = -1;
};

class AmrNbCodec : public Codec
{
protected:
    void* mEncoderCtx = nullptr;
    void* mDecoderCtx = nullptr;
    AmrCodecConfig mConfig;
    unsigned mCurrentDecoderTimestamp = 0;
    int mPreviousPacketLength = 0;
    size_t mCngCounter = 0;
    size_t mSwitchCounter = 0;
public:
    class CodecFactory: public Factory
    {
    public:
        CodecFactory(const AmrCodecConfig& config);

        const char* name() override;
        int samplerate() override;
        int payloadType() override;

        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        void create(CodecMap& codecs) override;

        PCodec create() override;

    protected:
        AmrCodecConfig mConfig;
    };

    AmrNbCodec(const AmrCodecConfig& config);
    ~AmrNbCodec();

    Info info() override;

    EncodeResult encode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    DecodeResult decode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    size_t plc(int lostFrames, std::span<uint8_t> output) override;

    int getSwitchCounter() const;
    int getCngCounter() const;
};

struct AmrWbStatistics
{
    int mDiscarded = 0;
    int mNonParsed = 0;
};
extern AmrWbStatistics GAmrWbStatistics;

class AmrWbCodec : public Codec
{
protected:
    void* mEncoderCtx = nullptr;
    void* mDecoderCtx = nullptr;
    AmrCodecConfig mConfig;
    uint64_t mCurrentDecoderTimestamp = 0;
    size_t mSwitchCounter = 0;
    size_t mCngCounter = 0;

    int mPreviousPacketLength;

    DecodeResult decodeIuup(std::span<const uint8_t> input, std::span<uint8_t> output);
    DecodeResult decodePlain(std::span<const uint8_t> input, std::span<uint8_t> output);

public:
    class CodecFactory: public Factory
    {
    public:
        CodecFactory(const AmrCodecConfig& config);

        const char* name() override;
        int samplerate() override;
        int payloadType() override;

        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        void create(CodecMap& codecs) override;

        PCodec create() override;

    protected:
        AmrCodecConfig mConfig;
    };

    AmrWbCodec(const AmrCodecConfig& config);
    virtual ~AmrWbCodec();

    Info info() override;

    EncodeResult encode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    DecodeResult decode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    size_t       plc(int lostFrames, std::span<uint8_t> output) override;
    int getSwitchCounter() const;
    int getCngCounter() const;
};

class GsmEfrCodec : public Codec
{
protected:
    void* mEncoderCtx = nullptr;
    void* mDecoderCtx = nullptr;
    bool mIuUP = false;

public:
    class GsmEfrFactory: public Factory
    {
    public:
        GsmEfrFactory(bool iuup, int ptype);

        const char* name() override;
        int samplerate() override;
        int payloadType() override;

        void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
        void create(CodecMap& codecs) override;

        PCodec create() override;
    protected:
        bool mIuUP;
        int mPayloadType;
    };

    GsmEfrCodec(bool iuup = false);
    ~GsmEfrCodec();

    Info info() override;

    EncodeResult encode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    DecodeResult decode(std::span<const uint8_t> input, std::span<uint8_t> output) override;
    size_t       plc(int lostFrames, std::span<uint8_t> output) override;
};

} // End of MT namespace


#endif // MT_AMRCODEC_H

