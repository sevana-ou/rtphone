/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_RTP_H
#define __HL_RTP_H

#if defined(USE_RTPDUMP)
# include "jrtplib/src/rtppacket.h"
#endif

#if !defined(USE_NULL_UUID)
# include "HL_Uuid.h"
#endif


#include "HL_InternetAddress.h"

#include <vector>
#include <string>

// Class to carry rtp/rtcp socket pair
template<class T>
struct RtpPair
{
    T mRtp;
    T mRtcp;

    RtpPair()
    {}

    RtpPair(const T& rtp, const T& rtcp)
        :mRtp(rtp), mRtcp(rtcp)
    {}

    bool multiplexed() { return mRtp == mRtcp; }
};

class RtpHelper
{
public:
    static bool isRtp(const void* buffer, size_t length);
    static int findPtype(const void* buffer, size_t length);
    static int findPacketNo(const void* buffer, size_t length);
    static bool isRtpOrRtcp(const void* buffer, size_t length);
    static bool isRtcp(const void* buffer, size_t length);
    static unsigned findSsrc(const void* buffer, size_t length);
    static int findPayloadLength(const void* buffer, size_t length);
};

#if defined(USE_RTPDUMP)
class RtpDump
{
protected:
    struct RtpData
    {
        jrtplib::RTPPacket* mPacket;
        void* mData;
        size_t mLength;
    };

    typedef std::vector<RtpData> PacketList;
    PacketList mPacketList;
    std::string mFilename;

public:
    RtpDump(const char* filename);
    ~RtpDump();

    void load();
    size_t count() const;
    jrtplib::RTPPacket& packetAt(size_t index);
    void add(const void* data, size_t len);
    void flush();
};
#endif

struct MediaStreamId
{
    InternetAddress mSource;
    InternetAddress mDestination;
    uint32_t mSSRC = 0;
    bool mSsrcIsId = true;
#if !defined(USE_NULL_UUID)
    Uuid mLinkId;
#endif
    bool operator < (const MediaStreamId& s2) const;
    bool operator == (const MediaStreamId& right) const;

    std::string toString() const;
    std::string getDetectDescription() const;
    std::string getFinishDescription() const;
};

#endif
