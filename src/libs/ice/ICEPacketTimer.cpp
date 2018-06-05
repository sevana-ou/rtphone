/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICEPacketTimer.h"
#include "ICETime.h"

using namespace ice;
PacketScheduler::PacketScheduler()
{
  mTimestamp = 0;
  mAttemptCounter = 0;
  mInitialRTO = 100;
  mLastRTO = 100;
}

PacketScheduler::~PacketScheduler()
{
}

void PacketScheduler::setInitialRTO(int value)
{
  mInitialRTO = value;
}

int  PacketScheduler::initialRTO()
{
  return mInitialRTO;
}

void PacketScheduler::start()
{
  mLastRTO = mInitialRTO;
  mAttemptCounter = 0;
  mTimestamp = 0;//ICETimeHelper::timestamp();
}

void PacketScheduler::stop()
{
  ;  
}

bool PacketScheduler::isTimeToRetransmit()
{
  if (!mTimestamp)
  {
    mTimestamp = ICETimeHelper::timestamp();
    return true;
  }

  unsigned currentTime = ICETimeHelper::timestamp();
  unsigned delta =  ICETimeHelper::findDelta(mTimestamp, currentTime);
  
  return delta >= mLastRTO;
}
  
/// Instructs timer that attempt made
void PacketScheduler::attemptMade()
{
  // Increase attempt counter
  mAttemptCounter++;

  // Save new timestamp
  mTimestamp = ICETimeHelper::timestamp();
  
  // Increase mLastRTO
#ifndef ICE_SIMPLE_SCHEDULE  
  mLastRTO *= 2;
#endif
}

/// Checks if attempt limit reached
bool PacketScheduler::isAttemptLimitReached()
{
  return mAttemptCounter >= ICE_TRANSACTION_RTO_LIMIT;
}
  
/// Checks if timeout happens
bool PacketScheduler::isTimeout()
{
  // Are attempts to send finished?
  if (!isAttemptLimitReached())
    return false;
  
  // Get current time
  unsigned int currentTime = ICETimeHelper::timestamp();
  
  // Get difference between current time and last send attempt
  unsigned int delta = ICETimeHelper::findDelta(mTimestamp, currentTime);

  return delta > mLastRTO * 16;
}
