/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_TIME_H
#define __ICE_TIME_H

#include "ICEPlatform.h"
#include "ICETypes.h"
#ifndef _WIN32
#   include <time.h>
#   include <stdlib.h>
#endif

namespace ice {

  class TimeoutDetector
  {
  public:
    TimeoutDetector();
    ~TimeoutDetector();
    
    void start();
    void stop();
    
    /*! Resets the countdown. */
    void reset();

    /*! Checks if it is a time to retransmit non-answered packet
     *  @return true if time to retransmit packet, false if not. */
    bool isTimeToRetransmit();
  
    /*! Notifies timer about retransmission. */
    void nextAttempt();
  
    /*! Checks if it is a time to mark the transaction timeouted.
     *  @return true if transaction is timeouted, false if not. */
    bool isTimeout();
  protected:
  
    /// Time tracking member  
    time_t    mTimestamp;

    /// Retransmission attempt counter
    size_t    mAttempt;
  };

  class ICEStartTimer
  {
  public:
    ICEStartTimer();
    ~ICEStartTimer();

    void start(unsigned interval);
    bool isTimeToBegin();
    void stop();

  protected:
    unsigned  mTimestamp;
    unsigned  mInterval;
    bool      mEnabled;
  };
  
  /*! Class intendend to schedule connectivity checks. It is simple timer which returns true from IsTimeToSend() every interval milliseconds. */
  class ICEScheduleTimer
  {
  public:
    /*! Default constructor. */
    ICEScheduleTimer();
    
    /*! Destructor. */
    ~ICEScheduleTimer();

    /*! Start timer with interval in milliseconds. */
    void start(int interval);
    
    /*! Check if it is a time to send next check. This method must be called in while (IsTimeToSend()) {} loop - 
     *  as multiple checks can be scheduled for elapsed time interval. */
    bool isTimeToSend();
    
    /*! Stops timer. */
    void stop();
    
    /*! Returns remaining time in milliseconds. */
    int remaining();
    
    /*! Returns interval time in milliseconds. */
    int interval() const;
    
    /*! Sets new interval time in milliseconds. Can be called on active timer, will reschedule timeout event. */
    void setInterval(int interval);
    
  protected:
    bool      mEnabled;             /// Marks if timer is started
    unsigned  mTimestamp;           /// Last timestamp
    int       mTail;                /// How much checks was scheduled for elapsed time interval.
    int       mInterval;            /// Timer interval
  };

  class ICETimeHelper
  {
  public:
    static unsigned timestamp();
    static unsigned findDelta(unsigned oldTimestamp, unsigned int newTimestamp);
  };

};




#endif
