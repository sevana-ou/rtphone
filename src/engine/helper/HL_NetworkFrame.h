#ifndef _HL_NETWORK_FRAME_H
#define _HL_NETWORK_FRAME_H

#include <stdint.h>
#include "HL_InternetAddress.h"

class NetworkFrame
{
public:
  struct PacketData
  {
    const uint8_t* mData;
    int mLength;

    PacketData(const uint8_t* data, int length)
      :mData(data), mLength(length)
    {}

    PacketData()
      :mData(nullptr), mLength(0)
    {}
  };

  static PacketData GetUdpPayloadForEthernet(PacketData& packet, InternetAddress& source, InternetAddress& destination);
  static PacketData GetUdpPayloadForIp4(PacketData& packet, InternetAddress& source, InternetAddress& destination);
  static PacketData GetUdpPayloadForIp6(PacketData& packet, InternetAddress& source, InternetAddress& destination);
  static PacketData GetUdpPayloadForSLL(PacketData& packet, InternetAddress& source, InternetAddress& destination);

  struct EthernetHeader
  {
    /* Ethernet addresses are 6 bytes */
    static const int AddressLength = 6;
    uint8_t   mEtherDHost[AddressLength]; /* Destination host address */
    uint8_t   mEtherSHost[AddressLength]; /* Source host address */
    uint16_t  mEtherType; /* IP? ARP? RARP? etc */
  };

#if defined(TARGET_WIN)
  struct /*__attribute__((packed))*/ LinuxSllHeader
#else
  struct __attribute__((packed)) LinuxSllHeader
#endif
  {
    uint16_t mPacketType;
    uint16_t mARPHRD;
    uint16_t mAddressLength;
    uint64_t mAddress;
    uint16_t mProtocolType;
  };

  struct VlanHeader
  {
    uint16_t mMagicId;
    uint16_t mData;
  };

  struct Ip4Header 
  {
    uint8_t   mVhl;		                      /* version << 4 | header length >> 2 */
    uint8_t   mTos;		                      /* type of service */
    uint16_t  mLen;		                      /* total length */
    uint16_t  mId;		                      /* identification */
    uint16_t  mOffset;		                  /* fragment offset field */
#define IP_RF 0x8000		                    /* reserved fragment flag */
#define IP_DF 0x4000		                    /* dont fragment flag */
#define IP_MF 0x2000		                    /* more fragments flag */
#define IP_OFFMASK 0x1fff	                  /* mask for fragmenting bits */
    uint8_t   mTtl;		                      /* time to live */
    uint8_t   mProtocol;		                /* protocol */
    uint16_t  mChecksum;		/* checksum */
    in_addr   mSource, 
              mDestination; /* source and dest address */

    int headerLength() const 
    {
      return (mVhl & 0x0f) * 4;
    }

    int version() const
    {
      return mVhl >> 4;
    }

    const in_addr& source4() const  { return mSource; }
    const in_addr& dest4() const    { return mDestination; }
    const in6_addr& source6() const { return (const in6_addr&)mSource; }
    const in6_addr& dest6() const   { return (const in6_addr&)mDestination; }
  };

  struct UdpHeader
  {
    uint16_t	mSourcePort;		        /* source port */
    uint16_t  mDestinationPort;
    uint16_t	mDatagramLength;		    /* datagram length */
    uint16_t  mDatagramChecksum;			/* datagram checksum */
  };


  struct TcpHeader
  {
    uint16_t mSourcePort;	/* source port */
    uint16_t mDestinationPort;	/* destination port */
    uint32_t mSeqNo;		/* sequence number */
    uint32_t mAckNo;		/* acknowledgement number */
    uint32_t mDataOffset;	/* data offset, rsvd */
#define TH_OFF(th)	(((th)->th_offx2 & 0xf0) >> 4)
    uint8_t  mFlags;
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLAGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
    uint16_t mWindow;		/* window */
    uint16_t mChecksum;		/* checksum */
    uint16_t mUrgentPointer;		/* urgent pointer */
  };

};
#endif
