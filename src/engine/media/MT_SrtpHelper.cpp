/* Copyright(C) 2007-2025 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../engine_config.h"
#include "MT_SrtpHelper.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Rtp.h"
#include <openssl/rand.h>
#include <assert.h>
#include <format>

BiMap<SrtpSuite, std::string_view> SrtpSuiteNames{{
        {SRTP_AES_256_AUTH_80,      "AES_CM_256_HMAC_SHA1_80"},
        {SRTP_AES_128_AUTH_80,      "AES_CM_128_HMAC_SHA1_80"},
        {SRTP_AES_192_AUTH_80,      "AES_CM_192_HMAC_SHA1_80"},
        {SRTP_AES_256_AUTH_32,      "AES_CM_256_HMAC_SHA1_32"},
        {SRTP_AES_192_AUTH_32,      "AES_CM_192_HMAC_SHA1_32"},
        {SRTP_AES_128_AUTH_32,      "AES_CM_128_HMAC_SHA1_32"},
        {SRTP_AES_128_AUTH_NULL,    "AES_CM_128_NULL_AUTH"},
        {SRTP_AED_AES_256_GCM,      "AEAD_AES_256_GCM"},
        {SRTP_AED_AES_128_GCM,      "AEAD_AES_128_GCM"}}};

extern SrtpSuite   toSrtpSuite(const std::string_view& s)
{
    auto* suite = SrtpSuiteNames.find_by_value(s);
    return !suite ? SRTP_NONE : *suite;
}

extern std::string_view toString(SrtpSuite suite)
{
    auto* s = SrtpSuiteNames.find_by_key(suite);
    return s ? *s : std::string_view();
}

typedef void (*set_srtp_policy_function) (srtp_crypto_policy_t*);

set_srtp_policy_function findPolicyFunction(SrtpSuite suite)
{
    switch (suite)
    {
    case SRTP_AES_128_AUTH_80:      return &srtp_crypto_policy_set_rtp_default; break;
    case SRTP_AES_192_AUTH_80:      return &srtp_crypto_policy_set_aes_cm_192_hmac_sha1_80; break;
    case SRTP_AES_256_AUTH_80:      return &srtp_crypto_policy_set_aes_cm_256_hmac_sha1_80; break;
    case SRTP_AES_128_AUTH_32:      return &srtp_crypto_policy_set_aes_cm_128_hmac_sha1_32; break;
    case SRTP_AES_192_AUTH_32:      return &srtp_crypto_policy_set_aes_cm_192_hmac_sha1_32; break;
    case SRTP_AES_256_AUTH_32:      return &srtp_crypto_policy_set_aes_cm_256_hmac_sha1_32; break;
    case SRTP_AES_128_AUTH_NULL:    return &srtp_crypto_policy_set_aes_cm_128_null_auth; break;
    case SRTP_AED_AES_256_GCM:      return &srtp_crypto_policy_set_aes_gcm_256_16_auth; break;
    case SRTP_AED_AES_128_GCM:      return &srtp_crypto_policy_set_aes_gcm_128_16_auth; break;
    default:
        throw std::runtime_error(std::format("SRTP suite {} is not supported", toString(suite)));
    }
}

// --- SrtpStream ---
static void configureSrtpStream(SrtpStream& s, uint16_t ssrc, SrtpSuite suite)
{
    s.second.ssrc.type = ssrc_specific;
    s.second.ssrc.value = ntohl(ssrc);
    s.second.next = nullptr;
    set_srtp_policy_function func = findPolicyFunction(suite);
    if (!func)
        throw std::runtime_error(std::format("SRTP suite {} is not supported", toString(suite)));
    func(&s.second.rtp);
    func(&s.second.rtcp);
}

SrtpSession::SrtpSession()
    :mInboundSession(nullptr), mOutboundSession(nullptr)
{
    mSuite = SRTP_NONE;

    memset(&mInboundPolicy, 0, sizeof mInboundPolicy);
    mInboundPolicy.ssrc.type  = ssrc_specific;

    memset(&mOutboundPolicy, 0, sizeof mOutboundPolicy);
    mOutboundPolicy.ssrc.type = ssrc_specific;

    // Generate outgoing keys for all ciphers
    auto putKey = [this](SrtpSuite suite, size_t length){
        auto key = std::make_shared<ByteBuffer>();
        key->resize(length);
        RAND_bytes(key->mutableData(), key->size());
        mOutgoingKey[suite].first = key;
    };
    putKey(SRTP_AES_128_AUTH_80, 30); putKey(SRTP_AES_128_AUTH_32, 30);
    putKey(SRTP_AES_192_AUTH_80, 38); putKey(SRTP_AES_192_AUTH_32, 38);
    putKey(SRTP_AES_256_AUTH_80, 46); putKey(SRTP_AES_256_AUTH_32, 46);
    putKey(SRTP_AED_AES_128_GCM, 28);
    putKey(SRTP_AED_AES_256_GCM, 44);

}

SrtpSession::~SrtpSession()
{
}

/*void SrtpSession::addSsrc(unsigned ssrc, SsrcDirection d)
{
    Lock l(mGuard);
    assert(mSuite != SRTP_NONE);

    // Look in map - if the srtp stream for this ssrc is created already
    SrtpStreamMap::iterator streamIter;
    SrtpStream s;
    switch (d)
    {
    case sdIncoming:
        streamIter = mIncomingMap.find(ssrc);
        if (streamIter != mIncomingMap.end())
            return;

        configureSrtpStream(s, ssrc, mSuite);
        s.second.key = (unsigned char*)mIncomingKey.first->mutableData();
        mIncomingMap[ssrc] = s;
        srtp_add_stream(mInboundSession, &s.second);
        return;

    case sdOutgoing:
        streamIter = mOutgoingMap.find(ssrc);
        if (streamIter != mOutgoingMap.end())
            return;
        configureSrtpStream(s, ssrc, mSuite);
        s.second.key = (unsigned char*)mOutgoingKey[int(mSuite)-1].first->mutableData();
        mOutgoingMap[ssrc] = s;
        srtp_add_stream(mOutboundSession, &s.second);
        return;
    }
}*/

void SrtpSession::open(ByteBuffer& incomingKey, SrtpSuite suite)
{
    Lock l(mGuard);

    // Check if session is here already
    if (mInboundSession || mOutboundSession)
        return;

    // Save used SRTP suite
    mSuite = suite;

    // Save key
    mIncomingKey.first = std::make_shared<ByteBuffer>(incomingKey);

    auto policyFunction = findPolicyFunction(suite);
    if (!policyFunction)
        throw std::runtime_error(std::format("SRTP suite {} not found", toString(suite)));

    // Configure policies
    policyFunction(&mInboundPolicy.rtp);
    policyFunction(&mInboundPolicy.rtcp);
    policyFunction(&mOutboundPolicy.rtp);
    policyFunction(&mOutboundPolicy.rtcp);

    mOutboundPolicy.key = (unsigned char*)mOutgoingKey[int(suite)].first->mutableData();
    mOutboundPolicy.ssrc.type = ssrc_any_outbound;

    mInboundPolicy.key =  (unsigned char*)mIncomingKey.first->mutableData();
    mInboundPolicy.ssrc.type = ssrc_any_inbound;

    // Create SRTP session
    srtp_err_status_t err;

    err = srtp_create(&mOutboundSession, &mOutboundPolicy);
    if (err)
        throw Exception(ERR_SRTP, err);

    err = srtp_create(&mInboundSession, &mInboundPolicy);
    if (err)
        throw Exception(ERR_SRTP, err);
}

bool SrtpSession::active()
{
    Lock l(mGuard);
    return mInboundSession != 0 && mOutboundSession != 0;
}

void SrtpSession::close()
{
    Lock l(mGuard);

    if (mOutboundSession)
    {
        srtp_dealloc(mOutboundSession);
        mOutboundSession = nullptr;
    }

    if (mInboundSession)
    {
        srtp_dealloc(mInboundSession);
        mInboundSession = nullptr;
    }
}

SrtpKeySalt& SrtpSession::outgoingKey(SrtpSuite suite)
{
    Lock l(mGuard);
    assert(suite > SRTP_NONE && suite <= SRTP_LAST);
    return mOutgoingKey[int(suite)-1];
}

bool SrtpSession::protectRtp(void* buffer, int* length)
{
    // addSsrc(RtpHelper::findSsrc(buffer, *length), sdOutgoing);

    Lock l(mGuard);
    if (mOutboundSession)
    {
        size_t srtp_len = MAX_VALID_UDPPACKET_SIZE;
        auto code = srtp_protect(mOutboundSession,
                                 (const uint8_t*)buffer, (size_t)*length,
                                 (uint8_t*)buffer, &srtp_len,
                                 0 /* mki_index, non-used */);
        *length = srtp_len;
        return code == 0;
    }
    else
        return false;
}

bool SrtpSession::protectRtcp(void* buffer, int* length)
{
    //addSsrc(RtpHelper::findSsrc(buffer, *length), sdOutgoing);

    Lock l(mGuard);
    if (mOutboundSession)
    {
        size_t srtp_len = MAX_VALID_UDPPACKET_SIZE;
        auto code = srtp_protect_rtcp(mOutboundSession,
                                      (const uint8_t*)buffer, (size_t)*length,
                                      (uint8_t*)buffer, &srtp_len,
                                      0 /* mki_index, non-used */);
        *length = srtp_len;
        return code == 0;
    }
    else
        return false;
}

bool SrtpSession::unprotectRtp(const void* src, size_t srcLength, void* dst, size_t* dstLength)
{
    //addSsrc(RtpHelper::findSsrc(buffer, *length), sdIncoming);

    Lock l(mGuard);
    if (mInboundSession)
    {
        auto code = srtp_unprotect(mInboundSession,
                                   (const uint8_t*)src, srcLength,
                                   (uint8_t*)dst, dstLength);
        return code == 0;
    }
    else
        return false;
}

bool SrtpSession::unprotectRtcp(const void* src, size_t srcLength, void* dst, size_t* dstLength)
{
    //addSsrc(RtpHelper::findSsrc(buffer, *length), sdIncoming);

    Lock l(mGuard);
    if (mInboundSession)
    {
        auto code = srtp_unprotect_rtcp(mInboundSession,
                                        (const uint8_t*)src, srcLength,
                                        (uint8_t*)dst, dstLength);
        return code == 0;
    }
    else
        return false;
}

static bool GSrtpInitialized = false;
void SrtpSession::initSrtp()
{
    if (GSrtpInitialized)
        return;

    auto err = srtp_init();

    if (err != srtp_err_status_ok)
        throw Exception(ERR_SRTP, err);
    GSrtpInitialized = true;
}
