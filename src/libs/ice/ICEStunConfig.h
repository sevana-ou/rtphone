/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_STUN_CONFIG_H
#define __ICE_STUN_CONFIG_H


#include "ICEPlatform.h"
#include "ICECandidate.h"

#include <string>
#include <vector>

namespace ice {
  
#define DEFAULT_STUN_RETRANSMIT_TIMEOUT 790000
#define DEFAULT_STUN_FINISH_TIMEOUT     790000
#define ICE_FALLBACK_IP_ADDR              "8.8.8.8"
// #define TEST_RELAYING

  struct StackConfig
  {
    // The IP of STUN server
    std::vector<NetworkAddress> mServerList4, 
                                mServerList6;
    NetworkAddress mServerAddr4, 
                   mServerAddr6;
		
    // Use IPv4 when gathering candidates
    bool              mUseIPv4;

    // Use IPv6 when gathering candidates
    bool              mUseIPv6;

    // Should we use relying (TURN)?
    bool              mUseTURN;
    
    // Should we use IPv4 <--> IPv6 bypassing via relaying ?
    bool              mUseProtocolRelay;

    // The timeout for STUN transaction
    unsigned int      mTimeout;
    
    // The RTO
    unsigned int      mPeriod;
    
    // Marks if STUN use is disalbed
    bool              mUseSTUN;
    
    // Sets the possible peer IP for ICE session
    std::string       mTargetIP;
    
    // The type preference list for ICE session
    int               mTypePreferenceList[4];
    
    // The initial RTO value
    int               mInitialRTO;
    
    // Interval for keepalive checks
    int               mKeepAliveInterval;
    
    std::string       mTurnUsername,
                      mTurnPassword;
    
    // TURN lifetime
    int               mTurnLifetime;

    // Enable/disable aggressive nomination
    bool              mAggressiveNomination;

    // Treats requests as confirmation for connectivity checks sent in reverse direction if the corresponding pair already exists
    // It violates RFC. It can be needed in poor networks.
    bool              mTreatRequestAsConfirmation;
    
    // Default IP target if mTargetIP is not set
    static std::string mDefaultIpTarget;
    
    StackConfig();
    ~StackConfig();
  };

};

#endif
