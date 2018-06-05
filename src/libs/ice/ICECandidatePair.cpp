/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICECandidatePair.h"
#include "ICEBinding.h"
#include "ICERelaying.h"
#include "ICELog.h"
#include <stdio.h>

using namespace ice;
#define LOG_SUBSYSTEM "ICE"
CandidatePair::CandidatePair()
:mPriority(0), mState(CandidatePair::Frozen), mControlledIndex(0), mControllingIndex(1),
mNomination(Nomination_None), mRole(Regular), mTransaction(NULL)
{
  memset(mFoundation, 0, sizeof mFoundation);
}

CandidatePair::CandidatePair(const CandidatePair& src)
:mPriority(src.mPriority), mState(src.mState), mControlledIndex(src.mControlledIndex),
mControllingIndex(src.mControllingIndex), mNomination(src.mNomination),
mRole(src.mRole), mTransaction(src.mTransaction)
{
  mCandidate[0] = src.mCandidate[0];
  mCandidate[1] = src.mCandidate[1];
  memcpy(mFoundation, src.mFoundation, sizeof mFoundation);
}

CandidatePair& CandidatePair::operator = (const CandidatePair& src)
{
  mPriority = src.mPriority;
  mState = src.mState;
  memcpy(mFoundation, src.mFoundation, sizeof mFoundation);
  mControlledIndex = src.mControlledIndex;
  mControllingIndex = src.mControllingIndex;
  mNomination = src.mNomination;
  mTransaction = src.mTransaction;
  mRole = src.mRole;
  mCandidate[0] = src.mCandidate[0];
  mCandidate[1] = src.mCandidate[1];
  return *this;
}

CandidatePair::~CandidatePair()
{
}

const char* CandidatePair::stateToString(State s)
{
	switch (s)
	{
		case Waiting:		 return "Waiting";
		case InProgress: return "InProgress";
		case Succeeded:  return "Succeeded";
		case Failed:     return "Failed";
		case Frozen:     return "Frozen";	
	}
	return "UNEXPECTED";
}

const char* CandidatePair::nominationToString(ice::CandidatePair::Nomination n)
{
  switch (n)
  {
    case Nomination_None: return "nomination:none";
    case Nomination_Started: return "nomination:started";
    case Nomination_Finished: return "nomination:finished";
  }
  return "nomination:bad";
}

void CandidatePair::updatePriority()
{
  unsigned int G = mCandidate[mControllingIndex].mPriority;
  unsigned int D = mCandidate[mControlledIndex].mPriority;
  // As RFC says...
  mPriority = 0xFFFFFFFF * MINVALUE(G, D) + 2*MAXVALUE(G,D) + ((G>D)?1:0);
  
  // ICELogDebug(<< "G=" << G << ", D=" << D << ", priority=" << mPriority);
}

void CandidatePair::updateFoundation()
{
  // Get a combination of controlling and controlled foundations
  strcpy(mFoundation, mCandidate[mControlledIndex].mFoundation);
  strcat(mFoundation, mCandidate[mControlledIndex].mFoundation);
}

bool CandidatePair::operator == (const CandidatePair& rhs) const
{
  return this->mCandidate[0] == rhs.mCandidate[0] && this->mCandidate[1] == rhs.mCandidate[1];
}

const char* CandidatePair::roleToString(Role r)
{
  switch (r)
  {
  case Regular:   return "regular";
  case Triggered: return "triggered";
  case Valid:     return "valid";
	case None:      return "none";
  }
  return "UNEXPECTED";
}

std::string CandidatePair::toStdString()
{
  char result[256];
  
  sprintf(result, "(%s%s) %s %s -> %s %s ", roleToString(mRole), nominationToString(mNomination),
					mCandidate[0].type(), mCandidate[0].mLocalAddr.toStdString().c_str(), mCandidate[1].type(),
					mCandidate[1].mExternalAddr.toStdString().c_str());

  return result;
}

bool CandidatePair::isLanOnly() const
{
  return first().mLocalAddr.isLAN() &&
      second().mExternalAddr.isLAN();
}

CandidatePair::Role CandidatePair::role() const
{
  return mRole;
};

void CandidatePair::setRole(Role role)
{
  mRole = role;
}

CandidatePair::Nomination CandidatePair::nomination() const
{
  return mNomination;
}

void CandidatePair::setNomination(ice::CandidatePair::Nomination n)
{
  mNomination = n;
}

const char* CandidatePair::foundation() const
{
  return mFoundation;
};

void CandidatePair::setFoundation(const char* foundation)
{
  strcpy(mFoundation, foundation);
}

CandidatePair::State CandidatePair::state() const
{
  return mState;
}

void CandidatePair::setState(CandidatePair::State state)
{
  mState = state;
}

int64_t CandidatePair::priority() const
{
  return mPriority;
}

void CandidatePair::setPriority(int64_t priority)
{
  mPriority = priority;
}

Candidate& CandidatePair::first()
{
  return mCandidate[0];
}

const Candidate& CandidatePair::first() const
{
  return mCandidate[0];
}

Candidate& CandidatePair::second()
{
  return mCandidate[1];
}

const Candidate& CandidatePair::second() const
{
  return mCandidate[1];
}
unsigned CandidatePair::controlledIndex()
{
  return mControlledIndex;
}

void     CandidatePair::setControlledIndex(unsigned index)
{
  mControlledIndex = index;
}

unsigned CandidatePair::controllingIndex()
{
  return mControllingIndex;
}

void     CandidatePair::setControllingIndex(unsigned index)
{
  mControllingIndex = index;
}

void*  CandidatePair::transaction()
{
  return mTransaction;
}

void CandidatePair::setTransaction(void* t)
{
  mTransaction = t;
}
