/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_SrtpHelper.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Rtp.h"
#include <assert.h>

// --- SrtpStream ---
void initSrtpStream(SrtpStream& s, unsigned ssrc, SrtpSuite suite)
{
  s.second.ssrc.type = ssrc_specific;
  s.second.ssrc.value = ntohl(ssrc);
  s.second.next = NULL;
  switch (suite)
  {
  case SRTP_AES_128_AUTH_80:
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&s.second.rtp);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&s.second.rtcp);
    break;

  case SRTP_AES_256_AUTH_80:
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&s.second.rtp);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&s.second.rtcp);
    break;

  default:
    assert(0);
  }
}

SrtpSession::SrtpSession()
:mInboundSession(NULL), mOutboundSession(NULL)
{
  mInboundSession = NULL;
  mOutboundSession = NULL;
  mSuite = SRTP_NONE;

  memset(&mInboundPolicy, 0, sizeof mInboundPolicy);  
  mInboundPolicy.ssrc.type  = ssrc_specific;

  memset(&mOutboundPolicy, 0, sizeof mOutboundPolicy);
  mOutboundPolicy.ssrc.type = ssrc_specific;

  // Generate outgoing keys

  mOutgoingKey[SRTP_AES_128_AUTH_80-1].first = PByteBuffer(new ByteBuffer());
  mOutgoingKey[SRTP_AES_128_AUTH_80-1].first->resize(30);
  crypto_get_random((unsigned char*)mOutgoingKey[SRTP_AES_128_AUTH_80-1].first->mutableData(), 30);

  mOutgoingKey[SRTP_AES_256_AUTH_80-1].first = PByteBuffer(new ByteBuffer());
  mOutgoingKey[SRTP_AES_256_AUTH_80-1].first->resize(46);
  crypto_get_random((unsigned char*)mOutgoingKey[SRTP_AES_256_AUTH_80-1].first->mutableData(), 46);

}

SrtpSession::~SrtpSession()
{
}

void SrtpSession::addSsrc(unsigned ssrc, SsrcDirection d)
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
      
    initSrtpStream(s, ssrc, mSuite);
    s.second.key = (unsigned char*)mIncomingKey.first->mutableData();
    mIncomingMap[ssrc] = s;
    srtp_add_stream(mInboundSession, &s.second);
    return;

  case sdOutgoing:
    streamIter = mOutgoingMap.find(ssrc);
    if (streamIter != mOutgoingMap.end())
      return;
    initSrtpStream(s, ssrc, mSuite);
    s.second.key = (unsigned char*)mOutgoingKey[int(mSuite)-1].first->mutableData();
    mOutgoingMap[ssrc] = s;
    srtp_add_stream(mOutboundSession, &s.second);
    return;
  }
}

void SrtpSession::open(ByteBuffer& incomingKey, SrtpSuite suite)
{
  Lock l(mGuard);

  // Check if session is here already
  if (mInboundSession || mOutboundSession)
    return;

  // Save used SRTP suite
  mSuite = suite;
  
  // Save key
  mIncomingKey.first = PByteBuffer(new ByteBuffer(incomingKey));
  
  // Update policy
  switch (suite)
  {
  case SRTP_AES_128_AUTH_80:
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&mInboundPolicy.rtp);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&mInboundPolicy.rtcp);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&mOutboundPolicy.rtp);
    crypto_policy_set_aes_cm_128_hmac_sha1_80(&mOutboundPolicy.rtcp);
    break;

  case SRTP_AES_256_AUTH_80:
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&mInboundPolicy.rtp);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&mInboundPolicy.rtcp);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&mOutboundPolicy.rtp);
    crypto_policy_set_aes_cm_256_hmac_sha1_80(&mOutboundPolicy.rtcp);
    break;

  case SRTP_NONE:
    break;
  }
  mOutboundPolicy.key = (unsigned char*)mOutgoingKey[int(suite)-1].first->mutableData();
  mInboundPolicy.key = (unsigned char*)mIncomingKey.first->mutableData();
  
  // Create SRTP session
  err_status_t err;

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
    mOutboundSession = NULL;
  }

  if (mInboundSession)
  {
    srtp_dealloc(mInboundSession);
    mInboundSession = NULL;
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
  addSsrc(RtpHelper::findSsrc(buffer, *length), sdOutgoing);
  
  Lock l(mGuard);
  if (mOutboundSession)
    return srtp_protect(mOutboundSession, buffer, length) == 0;
  else
    return false;
}

bool SrtpSession::protectRtcp(void* buffer, int* length)
{
  addSsrc(RtpHelper::findSsrc(buffer, *length), sdOutgoing);

  Lock l(mGuard);
  if (mOutboundSession)
    return srtp_protect_rtcp(mOutboundSession, buffer, length) == 0;
  else
    return false;
}

bool SrtpSession::unprotectRtp(void* buffer, int* length)
{
  addSsrc(RtpHelper::findSsrc(buffer, *length), sdIncoming);

  Lock l(mGuard);
  if (mInboundSession)
    return srtp_unprotect(mInboundSession, buffer, length) == 0;
  else
    return false;
}

bool SrtpSession::unprotectRtcp(void* buffer, int* length)
{
  addSsrc(RtpHelper::findSsrc(buffer, *length), sdIncoming);

  Lock l(mGuard);
  if (mInboundSession)
    return srtp_unprotect_rtcp(mInboundSession, buffer, (int*)length) == 0;
  else
    return false;
}

static bool GSrtpInitialized = false;
void SrtpSession::initSrtp()
{
  if (GSrtpInitialized)
    return;

  err_status_t err = srtp_init();
  
  if (err != err_status_ok) 
    throw Exception(ERR_SRTP, err);
  GSrtpInitialized = true;
}
