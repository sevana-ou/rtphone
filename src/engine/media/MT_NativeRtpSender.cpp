/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_NativeRtpSender.h"
#include <assert.h>

using namespace MT;

NativeRtpSender::NativeRtpSender(Statistics& stat)
:mDumpWriter(NULL), mStat(stat), mSrtpSession(NULL)
{
}

NativeRtpSender::~NativeRtpSender()
{
}

bool NativeRtpSender::SendRTP(const void *data, size_t len)
{
  if (mTarget.mRtp.isEmpty() || !mSocket.mRtp)
    return false;
  
  if (mDumpWriter)
    mDumpWriter->add(data, len);

  // Copy data to intermediary buffer bigger that original
  int sendLength = len;
  memcpy(mSendBuffer, data, len);

  // Encrypt SRTP if needed
  if (mSrtpSession)
  {
    if (mSrtpSession->active())
    {
      if (!mSrtpSession->protectRtp(mSendBuffer, &sendLength))
        return false;
    }
  }

  mSocket.mRtp->sendDatagram(mTarget.mRtp, mSendBuffer, sendLength);
  mStat.mSentRtp++;
  mStat.mSent += len;

  return true;
}

/** This member function will be called when an RTCP packet needs to be transmitted. */
bool NativeRtpSender::SendRTCP(const void *data, size_t len)
{
  if (mTarget.mRtp.isEmpty() || !mSocket.mRtcp)
    return false;
  // Copy data to intermediary buffer bigger that original
  int sendLength = len;
  memcpy(mSendBuffer, data, len);

  // Encrypt SRTP if needed
  if (mSrtpSession)
  {
    if (mSrtpSession->active())
    {
      if (!mSrtpSession->protectRtcp(mSendBuffer, &sendLength))
        return false;
    }
  }

  mSocket.mRtcp->sendDatagram(mTarget.mRtcp, mSendBuffer, sendLength);
  mStat.mSentRtcp++;
  mStat.mSent += len;

  return true;
}


/** Used to identify if an RTPAddress instance originated from this sender (to be able to detect own packets). */
bool NativeRtpSender::ComesFromThisSender(const jrtplib::RTPAddress *a)
{
  return false;
}

void NativeRtpSender::setDestination(RtpPair<InternetAddress> target)
{
  mTarget = target;
}

RtpPair<InternetAddress> NativeRtpSender::destination()
{
  return mTarget;
}

void NativeRtpSender::setSocket(const RtpPair<PDatagramSocket>& socket)
{
  mSocket = socket;
}

RtpPair<PDatagramSocket>& NativeRtpSender::socket()
{
  return mSocket;
}

void NativeRtpSender::setDumpWriter(RtpDump *dump)
{
  mDumpWriter = dump;
}

RtpDump* NativeRtpSender::dumpWriter()
{
  return mDumpWriter;
}

void NativeRtpSender::setSrtpSession(SrtpSession* srtp)
{
  mSrtpSession = srtp;
}

SrtpSession* NativeRtpSender::srtpSession()
{
  return mSrtpSession;
}

