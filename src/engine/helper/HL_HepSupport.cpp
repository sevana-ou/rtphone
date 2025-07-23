#include "HL_HepSupport.h"

using namespace HEP;

static const uint32_t HEPID1 = 0x011002;
static const uint32_t HEPID2 = 0x021002;
static const uint32_t HEPID3 = 0x48455033;


bool Packet::parseV3(const ByteBuffer& packet)
{
    if (packet.size() < 30)
        return false;

    BufferReader r(packet);
    char signature[4];
    r.readBuffer(signature, 4);

    if (signature[0] != 'H' || signature[1] != 'E' || signature[2] != 'P' || signature[3] != '3')
        return false;

    // Total length
    int l = r.readUShort();
    l -= 6;

    InternetAddress sourceAddr4, destAddr4, sourceAddr6, destAddr6;
    uint16_t sourcePort = 0, destPort = 0;
    while (r.count() < packet.size())
    {
        mVendorId = (VendorId)r.readUShort();
        ChunkType chunkType = (ChunkType)r.readUShort();
        int chunkLength = r.readUShort();

        switch (chunkType)
        {
        case ChunkType::IPProtocolFamily:
            mIpProtocolFamily = r.readUChar();
            break;

        case ChunkType::IPProtocolID:
            mIpProtocolId = r.readUChar();
            break;

        case ChunkType::IP4SourceAddress:
            sourceAddr4 = r.readIp(AF_INET);
            break;

        case ChunkType::IP4DestinationAddress:
            destAddr4 = r.readIp(AF_INET);
            break;

        case ChunkType::IP6SourceAddress:
            sourceAddr6 = r.readIp(AF_INET);
            break;

        case ChunkType::IP6DestinationAddress:
            destAddr6 = r.readIp(AF_INET6);
            break;

        case ChunkType::SourcePort:
            sourcePort = r.readUShort();
            break;

        case ChunkType::DestinationPort:
            destPort = r.readUShort();
            break;

        case ChunkType::Timestamp:
            mTimestamp.tv_sec = r.readUInt();
            break;

        case ChunkType::TimestampMicro:
            mTimestamp.tv_usec = r.readUInt() * 1000;
            break;

        case ChunkType::ProtocolType:
            mProtocolType = (ProtocolId)r.readUChar();
            break;

        case ChunkType::CaptureAgentID:
            mCaptureAgentId = r.readUInt();
            break;

        case ChunkType::KeepAliveTimer:
            mKeepAliveTimer = r.readUShort();
            break;

        case ChunkType::AuthenticationKey:
            mAuthenticateKey.resize(chunkLength - 6);
            r.readBuffer(mAuthenticateKey.mutableData(), mAuthenticateKey.size());
            break;

        case ChunkType::PacketPayload:
            mBodyOffset = r.count();
            r.readBuffer(mBody, chunkLength - 6);
            break;

        default:
            r.readBuffer(nullptr, chunkLength - 6);
        }
    }

    if (!sourceAddr4.isEmpty())
        mSourceAddress = sourceAddr4;
    else
    if (!sourceAddr6.isEmpty())
        mSourceAddress = sourceAddr6;

    if (!mSourceAddress.isEmpty())
        mSourceAddress.setPort(sourcePort);

    if (!destAddr4.isEmpty())
        mDestinationAddress = destAddr4;
    else
    if (!destAddr6.isEmpty())
        mDestinationAddress = destAddr6;

    if (!mDestinationAddress.isEmpty())
        mDestinationAddress.setPort(destPort);

    return true;
}

bool Packet::parseV2(const ByteBuffer &packet)
{
    if (packet.size() < 31)
        return false;

    if (packet[0] != 0x02)
        return false;

    BufferReader r(packet);
    r.readBuffer(nullptr, 4);

    uint16_t sourcePort = r.readUShort();
    uint16_t dstPort = r.readUShort();
    mSourceAddress = r.readIp(AF_INET);
    mSourceAddress.setPort(sourcePort);
    mDestinationAddress = r.readIp(AF_INET);
    mDestinationAddress.setPort(dstPort);
    mTimestamp.tv_sec = r.readUInt();
    mTimestamp.tv_usec = r.readUInt() * 1000;
    mCaptureAgentId = r.readUShort();
    r.readBuffer(nullptr, 2);
    mBody.clear();
    mBodyOffset = r.count();
    r.readBuffer(mBody, 65536 - 28);
    return true;
}

#define WRITE_CHUNK_UCHAR(T, V)   {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(1); w.writeUChar((uint8_t)V);}
#define WRITE_CHUNK_USHORT(T, V)  {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(2); w.writeUShort((uint16_t)V);}
#define WRITE_CHUNK_UINT(T, V)    {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(4); w.writeUInt((uint32_t)V);}
#define WRITE_CHUNK_IP4(T, V)     {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(4); w.writeIp(V);}
#define WRITE_CHUNK_IP6(T, V)     {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(8); w.writeIp(V);}
#define WRITE_CHUNK_BUFFER(T, V)  {w.writeUShort((uint16_t)mVendorId); w.writeUShort((uint16_t)T); w.writeUShort(8); w.writeBuffer(V.data(), V.size());}

ByteBuffer Packet::buildV3()
{
    ByteBuffer r; r.resize(mBody.size() + 512);
    BufferWriter w(r);

    // Signature
    w.writeBuffer("HEP3", 4);

    // Reserve place for total length
    w.writeUShort(0);

    WRITE_CHUNK_UCHAR(ChunkType::IPProtocolFamily, mIpProtocolFamily);
    WRITE_CHUNK_UCHAR(ChunkType::IPProtocolID, mIpProtocolId);

    // Source address
    if (!mSourceAddress.isEmpty())
    {
        if (mSourceAddress.isV4())
            WRITE_CHUNK_IP4(ChunkType::IP4SourceAddress, mSourceAddress)
                    else
                    if (mSourceAddress.isV6())
                    WRITE_CHUNK_IP6(ChunkType::IP6SourceAddress, mSourceAddress);

        WRITE_CHUNK_USHORT(ChunkType::SourcePort, mSourceAddress.port());
    }

    // Destination address
    if (!mDestinationAddress.isEmpty())
    {
        if (mDestinationAddress.isV4())
            WRITE_CHUNK_IP4(ChunkType::IP4DestinationAddress, mDestinationAddress)
                    else
                    if (mDestinationAddress.isV6())
                    WRITE_CHUNK_IP6(ChunkType::IP6DestinationAddress, mDestinationAddress);

        WRITE_CHUNK_USHORT(ChunkType::DestinationPort, mDestinationAddress.port());
    }

    // Timestamp
    WRITE_CHUNK_UINT(ChunkType::Timestamp, mTimestamp.tv_sec);

    // TimestampMicro
    WRITE_CHUNK_UINT(ChunkType::TimestampMicro, mTimestamp.tv_usec / 1000);

    // Protocol type
    WRITE_CHUNK_UINT(ChunkType::ProtocolType, mProtocolType);

    // Capture agent ID
    WRITE_CHUNK_UINT(ChunkType::CaptureAgentID, mCaptureAgentId);

    // Keep alive timer value
    WRITE_CHUNK_USHORT(ChunkType::KeepAliveTimer, mKeepAliveTimer);

    // Authentication key
    WRITE_CHUNK_BUFFER(ChunkType::AuthenticationKey, mAuthenticateKey);

    // Payload
    WRITE_CHUNK_BUFFER(ChunkType::PacketPayload, mBody);

    r.resize(w.offset());

    w.rewind(); w.skip(4); w.writeUShort((uint16_t)r.size());

    return r;
}
