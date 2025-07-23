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
    unsigned char cc:4;         /* CSRC count             */
    unsigned char x:1;          /* header extension flag  */
    unsigned char p:1;          /* padding flag           */
    unsigned char version:2;	 /* protocol version       */
    unsigned char pt:7;	     /* payload type           */
    unsigned char m:1;		     /* marker bit             */
    unsigned short seq;		 /* sequence number        */
    unsigned int ts;		     /* timestamp              */
    unsigned int ssrc;	         /* synchronization source */
};

struct RtcpHeader
{
    unsigned char rc:5;		  /* reception report count */
    unsigned char p:1;          /* padding flag           */
    unsigned char version:2;    /* protocol version       */
    unsigned char pt:8;         /* payload type           */
    uint16_t len;               /* length                 */
    uint32_t ssrc;	       	     /* synchronization source */
};

bool RtpHelper::isRtp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;

    const RtpHeader* h = reinterpret_cast<const RtpHeader*>(buffer);
    if (h->version != 0b10)
        return false;

    unsigned char pt = h->pt;
    bool rtp = ( (pt & 0x7F) >= 96 && (pt & 0x7F) <= 127) || ((pt & 0x7F) < 35);
    return rtp;
}


bool RtpHelper::isRtpOrRtcp(const void* buffer, size_t length)
{
    if (length < 12)
        return false;
    const RtcpHeader* h = reinterpret_cast<const RtcpHeader*>(buffer);
    return h->version == 0b10;
}

bool RtpHelper::isRtcp(const void* buffer, size_t length)
{
    return (isRtpOrRtcp(buffer, length) && !isRtp(buffer, length));
}

unsigned RtpHelper::findSsrc(const void* buffer, size_t length)
{
    if (isRtp(buffer, length))
        return ntohl(reinterpret_cast<const RtpHeader*>(buffer)->ssrc);
    else
        return ntohl(reinterpret_cast<const RtcpHeader*>(buffer)->ssrc);
}

void RtpHelper::setSsrc(void* buffer, size_t length, uint32_t ssrc)
{
    if (isRtp(buffer, length))
        reinterpret_cast<RtpHeader*>(buffer)->ssrc = htonl(ssrc);
    else
        reinterpret_cast<RtcpHeader*>(buffer)->ssrc = htonl(ssrc);
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

