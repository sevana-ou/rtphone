/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEStunConfig.h"

using namespace ice;

std::string StackConfig::mDefaultIpTarget = ICE_FALLBACK_IP_ADDR;

StackConfig::StackConfig()
:mUseTURN(false), mTimeout(DEFAULT_STUN_FINISH_TIMEOUT), mPeriod(DEFAULT_STUN_RETRANSMIT_TIMEOUT),
mUseSTUN(true), mUseIPv4(true), mUseIPv6(true)
{
#ifdef ICE_AGGRESSIVE
    mAggressiveNomination = true;
    mTreatRequestAsConfirmation = false;
#endif
    
#ifdef ICE_VERYAGGRESSIVE
    mAggressiveNomination = true;
    mTreatRequestAsConfirmation = true;
#else
    mAggressiveNomination = false;
    mTreatRequestAsConfirmation = false;
#endif
    mInitialRTO = 100;
    mKeepAliveInterval = 5000;
    mServerAddr4.setPort( 3478 );
    mServerAddr6.setPort( 3478 );
    mTurnLifetime = 300;
    mUseProtocolRelay = true;// false;

#ifdef TEST_RELAYING
    mTypePreferenceList[Candidate::Host] = 0;
    mTypePreferenceList[Candidate::ServerReflexive] = 110;
    mTypePreferenceList[Candidate::PeerReflexive] = 100;
    mTypePreferenceList[Candidate::ServerRelayed] = 126;
#else
    mTypePreferenceList[Candidate::Host] = 126;
    mTypePreferenceList[Candidate::ServerReflexive] = 100;
    mTypePreferenceList[Candidate::PeerReflexive] = 110;
    mTypePreferenceList[Candidate::ServerRelayed] = 0;
#endif
    mTargetIP = mDefaultIpTarget;
}

StackConfig::~StackConfig()
{}
