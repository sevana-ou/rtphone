/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_SRTP_HELPER_H
#define __MT_SRTP_HELPER_H

#include <string>
#include <vector>
#include <map>

#include "libsrtp/include/srtp.h"
#include "HL_Sync.h"
#include "HL_ByteBuffer.h"
#include "HL_Types.h"

#define NAME_SRTP_AES_256_AUTH_80 "AES_CM_256_HMAC_SHA1_80"
#define NAME_SRTP_AES_128_AUTH_80 "AES_CM_128_HMAC_SHA1_80"

enum SrtpSuite
{
    SRTP_NONE,
    SRTP_AES_128_AUTH_80,
    SRTP_AES_256_AUTH_80,
    SRTP_AES_192_AUTH_80,
    SRTP_AES_128_AUTH_32,
    SRTP_AES_256_AUTH_32,
    SRTP_AES_192_AUTH_32,
    SRTP_AES_128_AUTH_NULL,
    SRTP_AED_AES_256_GCM,
    SRTP_AED_AES_128_GCM,
    SRTP_LAST = SRTP_AED_AES_128_GCM
    // ToDo:
    // a=crypto:1 AEAD_AES_256_GCM_8 inline:tN2A0vRjFBimpQsW2GasuJuPe7hKE26gki30APC8DVuySqCOYTs8lYBPR5I=
    // a=crypto:3 AEAD_AES_128_GCM_8 inline:Ok7VL8SmBHSbZLw4dK6iQgpliYKGdY9BHLJcRw==
};

extern SrtpSuite        toSrtpSuite(const std::string_view& s);
extern std::string_view toString(SrtpSuite suite);

typedef std::pair<PByteBuffer, PByteBuffer> SrtpKeySalt;
typedef std::pair<unsigned, srtp_policy_t> SrtpStream;

class SrtpSession
{
public:
    SrtpSession();
    ~SrtpSession();

    enum SsrcDirection
    {
        sdIncoming,
        sdOutgoing
    };
    SrtpKeySalt&     outgoingKey(SrtpSuite suite);

    void open(ByteBuffer& incomingKey, SrtpSuite suite);
    void close();
    bool active();

    /* bufferPtr is RTP packet data i.e. header + payload. Buffer must be big enough to hold encrypted data. */
    bool protectRtp(void* buffer, int* length);
    bool protectRtcp(void* buffer, int* length);
    bool unprotectRtp(const void* src, size_t srcLength, void* dst, size_t* dstLength);
    bool unprotectRtcp(const void* src, size_t srcLength, void* dst, size_t* dstLength);


    static void initSrtp();
protected:
    srtp_t          mInboundSession,
    mOutboundSession;

    SrtpKeySalt     mIncomingKey,
                    mOutgoingKey[SRTP_LAST];
    srtp_policy_t   mInboundPolicy;
    srtp_policy_t   mOutboundPolicy;
    SrtpSuite       mSuite;

    typedef std::map<unsigned, SrtpStream> SrtpStreamMap;
    SrtpStreamMap mIncomingMap, mOutgoingMap;
    Mutex mGuard;

    void addSsrc(unsigned ssrc, SsrcDirection d);
};

#endif
