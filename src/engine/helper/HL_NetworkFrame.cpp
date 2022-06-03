#include <iostream>

#include "HL_NetworkFrame.h"
#include "HL_InternetAddress.h"

#define ETHERTYPE_MPLS_UC   (0x8847)
#define ETHERTYPE_MPLS_MC   (0x8848)
#define ETHERTYPE_IPV6      (0x86dd)
#define ETHERTYPE_IP        (0x0800)

#define MPLS_STACK_MASK (0x00000100)
#define MPLS_STACK_SHIFT (8)

NetworkFrame::PacketData NetworkFrame::GetUdpPayloadForEthernet(NetworkFrame::PacketData& packet, InternetAddress& source, InternetAddress& destination)
{
    PacketData result(packet);

    const EthernetHeader* ethernet = reinterpret_cast<const EthernetHeader*>(packet.mData);

    // Skip ethernet header
    packet.mData += sizeof(EthernetHeader);
    packet.mLength -= sizeof(EthernetHeader);

    // See if there is Vlan header
    uint16_t proto = 0;
    if (ethernet->mEtherType == 129)
    {
        // Skip 1 or more VLAN headers
        do
        {
            const VlanHeader* vlan = reinterpret_cast<const VlanHeader*>(packet.mData);
            packet.mData += sizeof(VlanHeader);
            packet.mLength -= sizeof(VlanHeader);
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
            bottomOfStack = ((ntohl(*(uint32_t*)(packet.mData - 4)) & MPLS_STACK_MASK) >> MPLS_STACK_SHIFT) != 0)
        {
            packet.mData += 4;
            packet.mLength -=4;
        }
        break;

    case ETHERTYPE_IP:
        //  Next IPv4 packet
        break;

    case ETHERTYPE_IPV6:
        // Next IPv6 packet
        break;
    }

    const Ip4Header* ip4 = reinterpret_cast<const Ip4Header*>(packet.mData);

    if (ip4->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
        return PacketData();


    switch (ip4->version())
    {
    case 4:
        return GetUdpPayloadForIp4(packet, source, destination);

    case 6:
        return GetUdpPayloadForIp6(packet, source, destination);

    default:
        return PacketData();
    }
}

NetworkFrame::PacketData NetworkFrame::GetUdpPayloadForSLL(NetworkFrame::PacketData& packet, InternetAddress& source, InternetAddress& destination)
{
    PacketData result(packet);

    if (packet.mLength < 16)
        return PacketData();

    const LinuxSllHeader* sll = reinterpret_cast<const LinuxSllHeader*>(packet.mData);

    packet.mData += sizeof(LinuxSllHeader);
    packet.mLength -= sizeof(LinuxSllHeader);

    switch (ntohs(sll->mProtocolType))
    {
    case 0x0800:
        return GetUdpPayloadForIp4(packet, source, destination);

    case 0x86DD:
        return GetUdpPayloadForIp6(packet, source, destination);

    default:
        return PacketData();
    }
}

NetworkFrame::PacketData NetworkFrame::GetUdpPayloadForLoopback(NetworkFrame::PacketData& packet, InternetAddress& source, InternetAddress& destination)
{
    PacketData result(packet);

    if (packet.mLength < 16)
        return PacketData();

    struct LoopbackHeader
    {
        uint32_t mProtocolType;
    };

    const LoopbackHeader* lh = reinterpret_cast<const LoopbackHeader*>(packet.mData);

    packet.mData += sizeof(LoopbackHeader);
    packet.mLength -= sizeof(LoopbackHeader);

    switch (lh->mProtocolType)
    {
    case AF_INET:
        return GetUdpPayloadForIp4(packet, source, destination);

    case AF_INET6:
        return GetUdpPayloadForIp6(packet, source, destination);

    default:
        return PacketData();
    }
}

NetworkFrame::PacketData NetworkFrame::GetUdpPayloadForIp4(NetworkFrame::PacketData& packet, InternetAddress& source, InternetAddress& destination)
{
    PacketData result(packet);
    const Ip4Header* ip4 = reinterpret_cast<const Ip4Header*>(packet.mData);
    if (ip4->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
        return PacketData(nullptr, 0);

    result.mData += ip4->headerLength();
    result.mLength -= ip4->headerLength();

    const UdpHeader* udp = reinterpret_cast<const UdpHeader*>(result.mData);
    result.mData += sizeof(UdpHeader);
    result.mLength -= sizeof(UdpHeader);

    // Check if UDP payload length is smaller than full packet length. It can be VLAN trailer data - we need to skip it
    size_t length = ntohs(udp->mDatagramLength);
    if (length - sizeof(UdpHeader) < (size_t)result.mLength)
        result.mLength = length - sizeof(UdpHeader);

    source.setIp(ip4->mSource);
    source.setPort(ntohs(udp->mSourcePort));

    destination.setIp(ip4->mDestination);
    destination.setPort(ntohs(udp->mDestinationPort));

    return result;
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

NetworkFrame::PacketData NetworkFrame::GetUdpPayloadForIp6(NetworkFrame::PacketData& packet, InternetAddress& source, InternetAddress& destination)
{
    PacketData result(packet);
    const Ip6Header* ip6 = reinterpret_cast<const Ip6Header*>(packet.mData);
    /*if (ip6->mProtocol != IPPROTO_UDP && ip4->mProtocol != 0)
    return PacketData(nullptr, 0);
  */
    result.mData += sizeof(Ip6Header);
    result.mLength -= sizeof(Ip6Header);
    //std::cout << sizeof(Ip6Header) << std::endl;

    const UdpHeader* udp = reinterpret_cast<const UdpHeader*>(result.mData);
    result.mData += sizeof(UdpHeader);
    result.mLength -= sizeof(UdpHeader);

    /*
  if (result.mLength != ntohs(udp->mDatagramLength))
    return PacketData(nullptr, 0);
  */

    source.setIp(ip6->src_ip);
    source.setPort(ntohs(udp->mSourcePort));
    //std::cout << source.toStdString() << " - ";

    destination.setIp(ip6->dst_ip);
    destination.setPort(ntohs(udp->mDestinationPort));
    //std::cout << destination.toStdString() << std::endl;

    return result;
}
