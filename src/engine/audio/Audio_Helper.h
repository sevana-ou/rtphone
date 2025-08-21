/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_HELPER_H
#define __AUDIO_HELPER_H

#ifdef TARGET_WIN
#include <EndpointVolume.h>
#include <MMDeviceAPI.h>
#if defined(_MSC_VER)
# include <Functiondiscoverykeys_devpkey.h>
#endif
#endif

#if defined(TARGET_OSX) || defined(TARGET_IOS)
#   include <AudioUnit/AudioUnit.h>
#   include <AudioToolbox/AudioConverter.h>
#   include <AudioToolbox/AudioServices.h>
#   include <mach/mach_time.h>
#endif

#include <vector>

#include "Audio_Interface.h"

namespace Audio
{
  class TimeSource
  {
  protected:
#ifdef TARGET_WIN
    LARGE_INTEGER mCounter;       /// Current value from QPC.
    LARGE_INTEGER mFreq;          /// Current frequency from QPC.
#endif

#if defined(TARGET_OSX) || defined(TARGET_IOS)
    uint64_t mTime;
    struct mach_timebase_info mTimebase;
    double mRatio;
#endif

    unsigned mQuantTime;          /// Used time quants length in milliseconds.
    unsigned mDepthTime;          /// Number of available time quants.
    unsigned mTailTime;           /// Not-accounted milliseconds.

  public:
    TimeSource(int quantTime, int nrOfQuants);
    ~TimeSource() = default;

    void start();
    void stop();
    unsigned time();
  };

  class StubTimer
  {
  public:
    StubTimer(int bufferTime, int bufferCount);
    ~StubTimer();
    void start();
    void stop();
    void waitForBuffer();

  protected:
    unsigned mBufferTime;
    unsigned mBufferCount;
    unsigned mCurrentTime;
    TimeSource     mTimeSource;
#ifdef TARGET_WIN
    HANDLE        mStubSignal;
#endif
    bool          mActive;
  };
}

#endif
