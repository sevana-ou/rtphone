#ifndef __HELPER_HEP_SUPPORT_H
#define __HELPER_HEP_SUPPORT_H

#include "HL_ByteBuffer.h"
#include "HL_InternetAddress.h"

namespace HEP
{
  enum class ChunkType
  {
    None = 0,
    IPProtocolFamily,
    IPProtocolID,
    IP4SourceAddress,
    IP4DestinationAddress,
    IP6SourceAddress,
    IP6DestinationAddress,
    SourcePort,
    DestinationPort,
    Timestamp,
    TimestampMicro,
    ProtocolType,   // Maps to Protocol Types below
    CaptureAgentID,
    KeepAliveTimer,
    AuthenticationKey,
    PacketPayload,
    CompressedPayload,
    InternalC
  };

  enum class VendorId
  {
    None,
    FreeSwitch,
    Kamailio,
    OpenSIPS,
    Asterisk,
    Homer,
    SipXecs
  };

  enum class ProtocolId
  {
    Reserved = 0,
    SIP,
    XMPP,
    SDP,
    RTP,
    RTCP,
    MGCP,
    MEGACO,
    M2UA,
    M3UA,
    IAX,
    H322,
    H321
  };

  struct Packet
  {
    bool parseV3(const ByteBuffer& packet);
    bool parseV2(const ByteBuffer& packet);
    ByteBuffer buildV3();

    uint8_t
      mIpProtocolFamily,
      mIpProtocolId;

    InternetAddress
      mSourceAddress,
      mDestinationAddress;

    timeval mTimestamp;
    ProtocolId mProtocolType;
    uint16_t mCaptureAgentId;
    uint16_t mKeepAliveTimer;
    ByteBuffer mAuthenticateKey;
    ByteBuffer mBody;
    VendorId mVendorId;
  };

}

#endif
