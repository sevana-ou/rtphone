/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(TARGET_WIN)
# include <WinSock2.h>
# include <Windows.h>
#endif

#include "HL_Rtp.h"
#include "HL_Exception.h"
#include "HL_String.h"

#if defined(USE_RTP_DUMP)
# include "jrtplib/src/rtprawpacket.h"
# include "jrtplib/src/rtpipv4address.h"
#endif

#if !defined(TARGET_WIN)
# include <alloca.h>
#endif

#include <sstream>
#include <tuple>

struct RtpHeader
{
    unsigned char cc:4;	/* CSRC count             */
    unsigned char x:1;		/* header extension flag  */
    unsigned char p:1;		/* padding flag           */
    unsigned char version:2;	/* protocol version       */
    unsigned char pt:7;	/* payload type           */
    unsigned char m:1;		/* marker bit             */
    unsigned short seq;		/* sequence number        */
    unsigned int ts;		/* timestamp              */
    unsigned int ssrc;	/* synchronization source */
};

struct RtcpHeader
{
    unsigned char rc:5;		/* reception report count */
    unsigned char p:1;		/* padding flag           */
    unsigned char version:2;	/* protocol version       */
    unsigned char pt:8;		/* payload type           */
    uint16_t len;			/* length                 */
    uint32_t ssrc;	       	/* synchronization source */
};

bool RtpHelper::isRtp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;

    unsigned char _type = reinterpret_cast<const RtpHeader*>(buffer)->pt;
    bool rtp = ( (_type & 0x7F) >= 96 && (_type & 0x7F) <= 127) || ((_type & 0x7F) < 35);
    return rtp;
}


bool RtpHelper::isRtpOrRtcp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;
    unsigned char b = ((const unsigned char*)buffer)[0];

    return (b & 0xC0 ) == 128;
}

bool RtpHelper::isRtcp(const void* buffer, size_t length)
{
    return (isRtpOrRtcp(buffer, length) && !isRtp(buffer, length));
}

unsigned RtpHelper::findSsrc(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
        return reinterpret_cast<const RtpHeader*>(buffer)->ssrc;
    else
        return reinterpret_cast<const RtcpHeader*>(buffer)->ssrc;
}

int RtpHelper::findPtype(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
        return reinterpret_cast<const RtpHeader*>(buffer)->pt;
    else
        return -1;
}

int RtpHelper::findPacketNo(const void *buffer, size_t length)
{
    if (isRtp(buffer, length))
        return ntohs(reinterpret_cast<const RtpHeader*>(buffer)->seq);
    else
        return -1;
}

int RtpHelper::findPayloadLength(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
    {
        return length - 12;
    }
    else
        return -1;
}

#if defined(USE_RTPDUMP)
RtpDump::RtpDump(const char *filename)
    :mFilename(filename)
{}

RtpDump::~RtpDump()
{
    flush();
    for (PacketList::iterator packetIter=mPacketList.begin(); packetIter!=mPacketList.end(); ++packetIter)
    {
        //free(packetIter->mData);
        delete packetIter->mPacket;
    }
}

void RtpDump::load()
{
    FILE* f = fopen(mFilename.c_str(), "rb");
    if (!f)
        throw Exception(ERR_WAVFILE_FAILED);

    while (!feof(f))
    {
        RtpData data;
        fread(&data.mLength, sizeof data.mLength, 1, f);
        data.mData = new char[data.mLength];
        fread(data.mData, 1, data.mLength, f);
        jrtplib::RTPIPv4Address addr(jrtplib::RTPAddress::IPv4Address);
        jrtplib::RTPTime t(0);
        jrtplib::RTPRawPacket* raw = new jrtplib::RTPRawPacket((unsigned char*)data.mData, data.mLength, &addr, t, true);
        data.mPacket = new jrtplib::RTPPacket(*raw);
        mPacketList.push_back(data);
    }
}

size_t RtpDump::count() const
{
    return mPacketList.size();
}

jrtplib::RTPPacket& RtpDump::packetAt(size_t index)
{
    return *mPacketList[index].mPacket;
}

void RtpDump::add(const void* buffer, size_t len)
{
    RtpData data;
    data.mData = malloc(len);
    memcpy(data.mData, buffer, len);
    data.mLength = len;

    jrtplib::RTPIPv4Address addr(jrtplib::RTPAddress::IPv4Address);
    jrtplib::RTPTime t(0);
    jrtplib::RTPRawPacket* raw = new jrtplib::RTPRawPacket((unsigned char*)const_cast<void*>(data.mData), data.mLength, &addr, t, true);
    data.mPacket = new jrtplib::RTPPacket(*raw);
    //delete raw;
    mPacketList.push_back(data);
}

void RtpDump::flush()
{
    FILE* f = fopen(mFilename.c_str(), "wb");
    if (!f)
        throw Exception(ERR_WAVFILE_FAILED);

    PacketList::iterator packetIter = mPacketList.begin();
    for (;packetIter != mPacketList.end(); ++packetIter)
    {
        RtpData& data = *packetIter;
        // Disabled for debugging only
        //fwrite(&data.mLength, sizeof data.mLength, 1, f);
        fwrite(data.mData, data.mLength, 1, f);
    }
    fclose(f);
}
#endif


#ifndef EXTERNAL_MEDIA_STREAM_ID
// -------------- MediaStreamId --------------------
bool MediaStreamId::operator < (const MediaStreamId& right) const
{
    if (mSsrcIsId)
        return std::tie(mSSRC, mSource, mDestination) < std::tie(right.mSSRC, right.mSource, right.mDestination);
    else
        return std::tie(mSource, mDestination) < std::tie(right.mSource, right.mDestination);

}

bool MediaStreamId::operator == (const MediaStreamId& right) const
{
    if (mSsrcIsId)
        return std::tie(mSSRC, mSource, mDestination) == std::tie(right.mSSRC, right.mSource, right.mDestination);
    else
        return std::tie(mSource, mDestination) == std::tie(right.mSource, right.mDestination);
}

std::string MediaStreamId::toString() const
{
    std::ostringstream oss;
    oss << "src: " << mSource.toStdString() <<
           " dst: " << mDestination.toStdString() <<
           " ssrc: " << StringHelper::toHex(mSSRC);
    return oss.str();
}


void writeToJson(const MediaStreamId& id, std::ostringstream& oss)
{
    oss << "  \"src\": \"" << id.mSource.toStdString() << "\"," << std::endl
        << "  \"dst\": \"" << id.mDestination.toStdString() << "\"," << std::endl
        << "  \"ssrc\": \"" << StringHelper::toHex(id.mSSRC) << "\"," << std::endl
#if !defined(USE_NULL_UUID)
        << "  \"link_id\": \"" << id.mLinkId.toString() << "\"" << std::endl
#endif
        ;
}

std::string MediaStreamId::getDetectDescription() const
{
    std::ostringstream oss;
    oss << "{\"event\": \"stream_detected\"," << std::endl;
    writeToJson(*this, oss);
    oss << "}";

    return oss.str();
}

std::string MediaStreamId::getFinishDescription() const
{
    std::ostringstream oss;
    oss << "{" << std::endl
        << "  \"event\": \"stream_finished\", " << std::endl;
    writeToJson(*this, oss);
    oss << "}";

    return oss.str();
}

MediaStreamId& MediaStreamId::operator = (const MediaStreamId& src)
{
    this->mDestination = src.mDestination;
    this->mSource = src.mSource;
    this->mLinkId = src.mLinkId;
    this->mSSRC = src.mSSRC;
    this->mSsrcIsId = src.mSsrcIsId;

    return *this;
}

std::ostream& operator << (std::ostream& output, const MediaStreamId& id)
{
    return (output << id.toString());
}
#endif

