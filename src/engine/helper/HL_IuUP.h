#ifndef HL_IUUP_H
#define HL_IUUP_H

#include <memory>

class IuUP
{
public:
  enum class PduType
  {
    DataWithCrc = 0,
    DataNoCrc = 1,
    ControlProc = 14
  };

  struct Frame
  {
    PduType mPduType;
    uint8_t mFrameNumber;
    uint8_t mFqc;
    uint8_t mRfci;
    uint8_t mHeaderCrc;
    bool mHeaderCrcOk;
    uint16_t mPayloadCrc;
    bool mPayloadCrcOk;
    const uint8_t* mPayload;
    uint16_t mPayloadSize;
  };

  /* Default value is false */
  static bool TwoBytePseudoheader;

  static bool parse(const uint8_t* packet, int size, Frame& result);
  static bool parse2(const uint8_t* packet, int size, Frame& result);
};


#endif // HL_IUUP_H

