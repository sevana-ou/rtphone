#include <iostream>

#include "HL_NetworkFrame.h"
#include "HL_InternetAddress.h"

#define ETHERTYPE_MPLS_UC   (0x8847)
#define ETHERTYPE_MPLS_MC   (0x8848)
#define ETHERTYPE_IPV6      (0x86dd)
#define ETHERTYPE_IP        (0x0800)

#define MPLS_STACK_MASK (0x00000100)
#define MPLS_STACK_SHIFT (8)

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForRaw(const Packet& data)
{
    const Ip4Header* ip4 = reinterpret_cast<const Ip4Header*>(data.mData);

    if (ip4->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
        return Payload();


    switch (ip4->version())
    {
    case 4:
        return GetUdpPayloadForIp4(data);

    case 6:
        return GetUdpPayloadForIp6(data);

    default:
        return Payload();
    }
}

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForEthernet(const Packet& data)
{
    Packet result(data);

    const EthernetHeader* ethernet = reinterpret_cast<const EthernetHeader*>(data.mData);

    // Skip ethernet header
    result.mData += sizeof(EthernetHeader);
    result.mLength -= sizeof(EthernetHeader);

    // See if there is Vlan header
    uint16_t proto = 0;
    if (ethernet->mEtherType == 129)
    {
        // Skip 1 or more VLAN headers
        do
        {
            const VlanHeader* vlan = reinterpret_cast<const VlanHeader*>(result.mData);
            result.mData += sizeof(VlanHeader);
            result.mLength -= sizeof(VlanHeader);
            proto = ntohs(vlan->mData);
        }
        while (proto == 0x8100);
    }

    // Skip MPLS headers
    switch (proto)
    {
    case ETHERTYPE_MPLS_UC:
    case ETHERTYPE_MPLS_MC:
        // Parse MPLS here until marker "bottom of mpls stack"
        for(bool bottomOfStack = false; !bottomOfStack;
            bottomOfStack = ((ntohl(*(uint32_t*)(result.mData - 4)) & MPLS_STACK_MASK) >> MPLS_STACK_SHIFT) != 0)
        {
            result.mData += 4;
            result.mLength -=4;
        }
        break;

    case ETHERTYPE_IP:
        //  Next IPv4 packet
        break;

    case ETHERTYPE_IPV6:
        // Next IPv6 packet
        break;
    }

    const Ip4Header* ip4 = reinterpret_cast<const Ip4Header*>(result.mData);

    if (ip4->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
        return Payload();


    switch (ip4->version())
    {
    case 4:
        return GetUdpPayloadForIp4(result);

    case 6:
        return GetUdpPayloadForIp6(result);

    default:
        return Payload();
    }
}

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForSLL(const Packet& data)
{
    Packet result(data);

    if (result.mLength < 16)
        return Payload();

    const LinuxSllHeader* sll = reinterpret_cast<const LinuxSllHeader*>(result.mData);

    result.mData += sizeof(LinuxSllHeader);
    result.mLength -= sizeof(LinuxSllHeader);

    switch (ntohs(sll->mProtocolType))
    {
    case 0x0800:
        return GetUdpPayloadForIp4(result);

    case 0x86DD:
        return GetUdpPayloadForIp6(result);

    default:
        return Payload();
    }
}

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForLoopback(const Packet& data)
{
    Packet result(data);

    if (result.mLength < 16)
        return Payload();

    struct LoopbackHeader
    {
        uint32_t mProtocolType;
    };

    const LoopbackHeader* lh = reinterpret_cast<const LoopbackHeader*>(result.mData);

    result.mData += sizeof(LoopbackHeader);
    result.mLength -= sizeof(LoopbackHeader);

    switch (lh->mProtocolType)
    {
    case AF_INET:
        return GetUdpPayloadForIp4(result);

    case AF_INET6:
        return GetUdpPayloadForIp6(result);

    default:
        return Payload();
    }
}

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForIp4(const Packet& data)
{
    Packet result(data);
    const Ip4Header* ip4 = reinterpret_cast<const Ip4Header*>(data.mData);
    if (ip4->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
        return Payload();

    result.mData += ip4->headerLength();
    result.mLength -= ip4->headerLength();

    const UdpHeader* udp = reinterpret_cast<const UdpHeader*>(result.mData);
    result.mData += sizeof(UdpHeader);
    result.mLength -= sizeof(UdpHeader);

    // Check if UDP payload length is smaller than full packet length. It can be VLAN trailer data - we need to skip it
    size_t length = ntohs(udp->mDatagramLength);
    if (length - sizeof(UdpHeader) < (size_t)result.mLength)
        result.mLength = length - sizeof(UdpHeader);

    InternetAddress addr_source;
    addr_source.setIp(ip4->mSource);
    addr_source.setPort(ntohs(udp->mSourcePort));

    InternetAddress addr_dest;
    addr_dest.setIp(ip4->mDestination);
    addr_dest.setPort(ntohs(udp->mDestinationPort));

    return {.data = result, .source = addr_source, .dest = addr_dest};
}

struct Ip6Header
{
#if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t     traffic_class_hi:4,
        version:4;
    uint8_t     flow_label_hi:4,
        traffic_class_lo:4;
    uint16_t	flow_label_lo;

#elif __BYTE_ORDER == __BIG_ENDIAN
    uint8_t			version:4,
        traffic_class_hi:4;
    uint8_t			traffic_class_lo:4,
        flow_label_hi:4;
    uint16_t	    flow_label_lo;
#else
# error "Please fix endianness defines"
#endif

    uint16_t		payload_len;
    uint8_t			next_header;
    uint8_t			hop_limit;

    struct	in6_addr	src_ip;
    struct	in6_addr	dst_ip;
};

NetworkFrame::Payload NetworkFrame::GetUdpPayloadForIp6(const Packet& data)
{
    Packet result(data);
    const Ip6Header* ip6 = reinterpret_cast<const Ip6Header*>(result.mData);
    /*if (ip6->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
    return PacketData(nullptr, 0);
  */
    result.mData += sizeof(Ip6Header);
    result.mLength -= sizeof(Ip6Header);
    //std::cout << sizeof(Ip6Header) << std::endl;

    const UdpHeader* udp = reinterpret_cast<const UdpHeader*>(result.mData);
    result.mData += sizeof(UdpHeader);
    result.mLength -= sizeof(UdpHeader);

    InternetAddress addr_source;
    addr_source.setIp(ip6->src_ip);
    addr_source.setPort(ntohs(udp->mSourcePort));

    InternetAddress addr_dest;
    addr_dest.setIp(ip6->dst_ip);
    addr_dest.setPort(ntohs(udp->mDestinationPort));

    return {.data = result, .source = addr_source, .dest = addr_dest};
}
