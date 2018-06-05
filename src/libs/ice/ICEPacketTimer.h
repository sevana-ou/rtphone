/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_PACKET_TIMER_H
#define __ICE_PACKET_TIMER_H

namespace ice 
{

class PacketScheduler
{
public:
  PacketScheduler();
  ~PacketScheduler();

  void setInitialRTO(int value);
  int  initialRTO();

  void start();
  void stop();
  bool isTimeToRetransmit();
  
  /// Instructs timer that attempt made
  void attemptMade();

  /// Checks if attempt limit reached
  bool isAttemptLimitReached();
  
  /// Checks if timeout happens
  bool isTimeout();

protected:
  unsigned int mTimestamp;
  unsigned int mAttemptCounter;
  unsigned int mInitialRTO;
  unsigned int mLastRTO;
};

} //end of namespace

#endif