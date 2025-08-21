/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef TARGET_WIN
# include <WinSock2.h>
#endif

#include <assert.h>
#include "Audio_Helper.h"
#include "../helper/HL_Exception.h"

using namespace Audio;

// --- QPCSource
TimeSource::TimeSource(int quantTime, int nrOfQuants)
{
#ifdef TARGET_WIN
  mCounter.QuadPart = 0;
#endif
#if defined(TARGET_OSX) || defined(TARGET_IOS)
  mach_timebase_info(&mTimebase);
  mRatio = ((double)mTimebase.numer / (double)mTimebase.denom) / 1000000;

#endif
  mQuantTime = quantTime;
  mDepthTime = quantTime * nrOfQuants;
  mTailTime = 0;
}

void TimeSource::start()
{
#ifdef TARGET_WIN
  if (!QueryPerformanceFrequency(&mFreq))
    throw Exception(ERR_QPC, GetLastError());
  if (!QueryPerformanceCounter(&mCounter))
    throw Exception(ERR_QPC, GetLastError());
#endif
}

void TimeSource::stop()
{
}

unsigned TimeSource::time()
{
#ifdef TARGET_WIN
    LARGE_INTEGER c;
  if (!QueryPerformanceCounter(&c))
    throw Exception(ERR_QPC, GetLastError());
  
  //find the f
  double f = (double)mFreq.QuadPart / 1000.0;
  
  //find the difference
  unsigned __int64 diff = c.QuadPart - mCounter.QuadPart;

  mCounter.QuadPart = c.QuadPart;
  
  diff = (unsigned __int64)((double)diff / f + 0.5); //get ms
  diff += mTailTime; 

  if (diff > mDepthTime)
  {
    mTailTime = 0;
    return mDepthTime;
  }
  else
  {
    mTailTime = (unsigned )(diff % (unsigned __int64)mQuantTime);
    unsigned int t = (unsigned )(diff / (unsigned __int64)mQuantTime);
    return t * mQuantTime;
  }
#endif
#if defined(TARGET_OSX) || defined(TARGET_IOS)
  uint64_t t = mach_absolute_time();
  uint64_t c = uint64_t((double)t * mRatio + 0.5);

  uint64_t diff = c - this->mTime + mTailTime;
  mTime = c;
  if (diff > mDepthTime)
  {
      mTailTime = 0;
      return mDepthTime;
  }
  else
  {
    mTailTime = diff % mQuantTime;
    uint64_t t = diff / mQuantTime;
    return t * mQuantTime;
  }
#endif
#if defined(TARGET_LINUX)
  assert(0);
#endif

#if defined(TARGET_ANDROID)
  assert(0);
#endif
  return 0;
}

// --- StubTimer ---
StubTimer::StubTimer(int bufferTime, int bufferCount)
:mBufferTime(bufferTime), mBufferCount(bufferCount), mTimeSource(bufferTime, bufferCount), mActive(false), mCurrentTime(0)
{
#ifdef TARGET_WIN
    mStubSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
#endif
}

StubTimer::~StubTimer()
{
#ifdef TARGET_WIN
  ::CloseHandle(mStubSignal);
#endif
}

void StubTimer::start()
{
  mTimeSource.start();
  mCurrentTime = mTimeSource.time();
  mActive = true;
}

void StubTimer::stop()
{
  mTimeSource.stop();
  mActive = false;
}

void StubTimer::waitForBuffer()
{
  if (!mActive)
    start();

  unsigned t = mTimeSource.time();
    
  while (!t)
  {
#ifdef TARGET_WIN
      ::WaitForSingleObject(mStubSignal, mBufferTime);
#endif
#if defined(TARGET_OSX) || defined(TARGET_IOS)
      usleep(100);
#endif
    t = mTimeSource.time();
  }
}
