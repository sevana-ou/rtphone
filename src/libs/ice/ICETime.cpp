/* Copyright(C) 2007-2017 Voipobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#else
# include <time.h>
# include <stdlib.h>
# include <sys/times.h>
# include <unistd.h>
#endif

#include "ICEPlatform.h"

#include "ICETime.h"
using namespace ice;


#ifdef _WIN32
static unsigned GetMilliseconds()
{
  return ::GetTickCount();
}
#else
static int ClocksPerSec = (int)sysconf(_SC_CLK_TCK);
static tms GlobalStartTimeStruct;
static clock_t GlobalStartTime = times(&GlobalStartTimeStruct);
static unsigned GetMilliseconds()
{
  tms t;
  clock_t result = times(&t) - GlobalStartTime;
  float ticksPerMilliseconds = ClocksPerSec / 1000.0;
  return unsigned( result / ticksPerMilliseconds );
}
#endif

TimeoutDetector::TimeoutDetector()
  :mTimestamp(0), mAttempt(0)
{
}

TimeoutDetector::~TimeoutDetector()
{
}

void TimeoutDetector::start()
{
  mTimestamp = ::GetMilliseconds();
}

void TimeoutDetector::stop()
{
}

void TimeoutDetector::reset()
{
  mAttempt = 0;
}

bool TimeoutDetector::isTimeToRetransmit()
{
  static time_t RetransmissionIntervalArray[] = { 100, 200, 200, 200, 200, 200 };
  
  //get current time
  time_t current = ::GetMilliseconds();

  //find the delta
  time_t delta = current - mTimestamp;

  //find a wait interval for the mAttempt value
  time_t referenceInterval = 0;
  
  size_t arrayLength = sizeof(RetransmissionIntervalArray) / sizeof(RetransmissionIntervalArray[0]);
  
  if (mAttempt < arrayLength )
    referenceInterval = RetransmissionIntervalArray[mAttempt];
  else
    referenceInterval = RetransmissionIntervalArray[arrayLength-1];

  if (delta >= referenceInterval)
    return true;

  return false;
}

void TimeoutDetector::nextAttempt()
{
  mAttempt++;
  mTimestamp = ::GetMilliseconds();
}

bool TimeoutDetector::isTimeout()
{
  // Get current time
  time_t current = ::GetMilliseconds();

  // Find the delta
  time_t delta = current - mTimestamp;

  return delta > ICE_TIMEOUT_VALUE;
}


ICEStartTimer::ICEStartTimer()
  :mTimestamp(0), mInterval(0), mEnabled(false)
{
}

ICEStartTimer::~ICEStartTimer()
{
}

void ICEStartTimer::start(unsigned interval)
{
  mInterval = interval;
  mTimestamp = ::GetMilliseconds();
  mEnabled = true;
}

void ICEStartTimer::stop()
{
  mEnabled = false;
}

bool ICEStartTimer::isTimeToBegin()
{
  if (!mEnabled)
    return false;

  unsigned t = ::GetMilliseconds();
  unsigned delta = 0;

  if (t < 0x7FFFFFFF && mTimestamp > 0x7FFFFFFF)
    delta = 0xFFFFFFFF - mTimestamp + t;
  else
    delta = t - mTimestamp;

  return delta >= mInterval;
}

ICEScheduleTimer::ICEScheduleTimer()
:mEnabled(false), mTimestamp(0), mTail(0), mInterval(0)
{
}

ICEScheduleTimer::~ICEScheduleTimer()
{
}

void ICEScheduleTimer::start(int interval)
{
  mEnabled = true;
  mInterval = interval;
  mTail = 1; // First check is immediate!
  mTimestamp = ICETimeHelper::timestamp();
}
    
bool ICEScheduleTimer::isTimeToSend()
{
  if (!mEnabled)
    return false;

#ifdef ICE_SIMPLE_SCHEDULE
  unsigned current = ICETimeHelper::timestamp();
  unsigned delta = ICETimeHelper::findDelta(mTimestamp, current);
  
  return delta > mInterval;
#else
  // Check if there are already scheduled checks
  if (mTail > 0)
  {
    mTail--;
    return true;
  }

  unsigned current = ICETimeHelper::timestamp();
  unsigned delta = ICETimeHelper::findDelta(mTimestamp, current);

  if (delta > (unsigned)mInterval)
  {
    mTail += delta / mInterval;
    mTimestamp = current - delta % mInterval;
  }
  
  if (mTail > 0)
  {
    mTail--;
    return true;
  }

  return false;
#endif
}
    
void ICEScheduleTimer::stop()
{
  mEnabled = false;
}

int ICEScheduleTimer::remaining()
{
  unsigned delta = ICETimeHelper::findDelta(mTimestamp, ICETimeHelper::timestamp());
#ifdef ICE_SIMPLE_SCHEDULE
  return (delta >= mInterval) ? 0 : mInterval - delta;
#else
  return ((int)delta + mTail >= mInterval) ? 0 : mInterval - delta - mTail;
#endif
}

int ICEScheduleTimer::interval() const
{
  return mInterval;
}

void ICEScheduleTimer::setInterval(int interval)
{
  mTail = 0;
  mInterval = interval;
}

unsigned int ICETimeHelper::timestamp()
{
  return ::GetMilliseconds();
}

unsigned int ICETimeHelper::findDelta(unsigned int oldTS, unsigned int newTS)
{
  return (oldTS > 0x7FFFFFFF && newTS < 0x7FFFFFFF) ? 0xFFFFFFFF - oldTS + newTS : newTS - oldTS;
}
