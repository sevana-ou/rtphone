#ifndef MT_EVSCODEC_H
#define MT_EVSCODEC_H

#include "../config.h"
#include <map>
#include "MT_Codec.h"
#include "../helper/HL_Pointer.h"

#if defined(USE_EVS_CODEC)
namespace MT
{
  struct AmrCodecConfig
  {
    bool mIuUP;
    bool mOctetAligned;
    int mPayloadType;
  };

  class AmrNbCodec : public Codec
  {
  protected:
    void* mEncoderCtx;
    void* mDecoderCtx;
    AmrCodecConfig mConfig;
    unsigned mCurrentDecoderTimestamp;
    int mSwitchCounter;
    int mPreviousPacketLength;

  public:
    class CodecFactory: public Factory
    {
    public:
      CodecFactory(const AmrCodecConfig& config);

      const char* name() override;
      int samplerate() override;
      int payloadType() override;

#if defined(USE_RESIP_INTEGRATION)
      void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      void create(CodecMap& codecs) override;
#endif
      PCodec create() override;

    protected:
      AmrCodecConfig mConfig;
    };

    AmrNbCodec(const AmrCodecConfig& config);

    virtual ~AmrNbCodec();
    const char* name() override;
    int pcmLength() override;
    int rtpLength() override;
    int frameTime() override;
    int samplerate() override;
    int encode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int decode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int plc(int lostFrames, void* output, int outputCapacity) override;
    int getSwitchCounter() const;
  };

  class AmrWbCodec : public Codec
  {
  protected:
    void* mEncoderCtx;
    void* mDecoderCtx;
    AmrCodecConfig mConfig;
    uint64_t mCurrentDecoderTimestamp;
    int mSwitchCounter;
    int mPreviousPacketLength;

  public:
    class CodecFactory: public Factory
    {
    public:
      CodecFactory(const AmrCodecConfig& config);

      const char* name() override;
      int samplerate() override;
      int payloadType() override;

#if defined(USE_RESIP_INTEGRATION)
      void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      void create(CodecMap& codecs) override;
#endif
      PCodec create() override;

    protected:
      AmrCodecConfig mConfig;
    };

    AmrWbCodec(const AmrCodecConfig& config);
    virtual ~AmrWbCodec();

    const char* name() override;
    int pcmLength() override;
    int rtpLength() override;
    int frameTime() override;
    int samplerate() override;
    int encode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int decode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int plc(int lostFrames, void* output, int outputCapacity) override;
    int getSwitchCounter() const;
  };

  class GsmEfrCodec : public Codec
  {
  protected:
    void* mEncoderCtx;
    void* mDecoderCtx;
    bool mIuUP;

  public:
    class GsmEfrFactory: public Factory
    {
    public:
      GsmEfrFactory(bool iuup, int ptype);

      const char* name() override;
      int samplerate() override;
      int payloadType() override;

#if defined(USE_RESIP_INTEGRATION)
      void updateSdp(resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      int processSdp(const resip::SdpContents::Session::Medium::CodecContainer& codecs, SdpDirection direction) override;
      void create(CodecMap& codecs) override;
#endif
      PCodec create() override;

    protected:
      bool mIuUP;
      int mPayloadType;
    };

    GsmEfrCodec(bool iuup = false);

    virtual ~GsmEfrCodec();
    const char* name() override;
    int pcmLength() override;
    int rtpLength() override;
    int frameTime() override;
    int samplerate() override;
    int encode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int decode(const void* input, int inputBytes, void* output, int outputCapacity) override;
    int plc(int lostFrames, void* output, int outputCapacity) override;
  };

} // End of MT namespace


#endif

#endif // MT_EVSCODE_H

