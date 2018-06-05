/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_Stream.h"
#include "../audio/Audio_Interface.h"
#include "../helper/HL_Log.h"
#include <math.h>

#define LOG_SUBSYSTEM "[Media]"

using namespace MT;



Stream::Stream()
:mState(0)
{
}

Stream::~Stream()
{

}

void Stream::setDestination(const RtpPair<InternetAddress>& dest)
{
  ICELogInfo(<< "Set RTP destination to " << dest.mRtp.toStdString());
  mDestination = dest;

  mStat.mRemotePeer = dest.mRtp;
}
    
void Stream::setState(unsigned state)
{
  mState = state;
}

unsigned Stream::state()
{
  return mState;
}

void Stream::setSocket(const RtpPair<PDatagramSocket>& socket)
{
  mSocket = socket;
}

RtpPair<PDatagramSocket>& Stream::socket()
{
  return mSocket;
}

Statistics& Stream::statistics()
{
  return mStat;
}

SrtpSession& Stream::srtp()
{
  return mSrtpSession;
}

void Stream::configureMediaObserver(MediaObserver *observer, void* userTag)
{
  mMediaObserver = observer;
  mMediaObserverTag = userTag;
}

StreamList::StreamList()
{
}

StreamList::~StreamList()
{
  clear();
}

void StreamList::add(PStream s)
{
  Lock l(mMutex);
  mStreamVector.push_back(s);
}

void StreamList::remove(PStream s)
{
  Lock l(mMutex);

  StreamVector::iterator streamIter = std::find(mStreamVector.begin(), mStreamVector.end(), s);
  if (streamIter != mStreamVector.end())
    mStreamVector.erase(streamIter);
}

void StreamList::clear()
{
  Lock l(mMutex);
  mStreamVector.clear();
}

bool StreamList::has(PStream s)
{
  Lock l(mMutex);
  return std::find(mStreamVector.begin(), mStreamVector.end(), s) != mStreamVector.end();
}

int StreamList::size()
{
  Lock l(mMutex);
  return mStreamVector.size();
}

PStream StreamList::streamAt(int index)
{
  return mStreamVector[index];
}

void StreamList::copyTo(StreamList* sl)
{
  Lock l(mMutex);
  Lock l2(sl->mMutex);
  StreamVector::iterator streamIter = mStreamVector.begin();
  for(;streamIter != mStreamVector.end(); ++streamIter)
    sl->add(*streamIter);
}

Mutex& StreamList::getMutex()
{
  return mMutex;
}