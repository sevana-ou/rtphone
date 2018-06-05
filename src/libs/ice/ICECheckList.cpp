/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICECheckList.h"
#include "ICENetworkHelper.h"
#include "ICELog.h"

#include <algorithm>
#include <iterator>
#include <sstream>
#include <strstream>

using namespace ice;
#define LOG_SUBSYSTEM "ICE"

const char* CheckList::stateToString(int state)
{
  switch (state)
  {
  case Running:   return "Running";
  case Completed: return "Completed";
  case Failed:    return "Failed";
  }
  return "Undefined";
}
CheckList::CheckList()
:mState(CheckList::Running)
{
}

CheckList::State CheckList::state()
{
  return mState;
}

void CheckList::setState(CheckList::State state)
{
  mState = state;
}

CheckList::PairList& CheckList::pairlist()
{
  return mPairList;
}

static bool ComparePairByPriority(const PCandidatePair& pair1, const PCandidatePair& pair2)
{
  return pair1->priority() > pair2->priority();
}

void CheckList::pruneDuplicates()
{
  // Remove duplicates
  for (PairList::iterator pairIter = mPairList.begin(); pairIter != mPairList.end(); pairIter++)
  {
    PCandidatePair& pair = *pairIter;
    
    // Get the iterator to next element
    PairList::iterator nextPairIter = pairIter; std::advance(nextPairIter, 1);
    
    // Iterate next elements to find duplicates
    while (nextPairIter != mPairList.end())
    {
      PCandidatePair& nextPair = *nextPairIter;
      
      bool sameType = pair->first().mLocalAddr.family() == nextPair->second().mLocalAddr.family();
      bool sameAddress = pair->second().mExternalAddr == nextPair->second().mExternalAddr;
      bool relayed1 = pair->first().mType == Candidate::ServerRelayed;
      bool relayed2 = nextPair->first().mType == Candidate::ServerRelayed;
      bool sameRelayed = relayed1 == relayed2;
#ifdef ICE_SMART_PRUNE_CHECKLIST
      if (sameType && sameAddress && sameRelayed)
#else
        if (equalCandidates && sameAddress && sameRelayed)
#endif
          nextPairIter = mPairList.erase(nextPairIter);
        else
          nextPairIter++;
    }
  }

}

void CheckList::prune(int /*checkLimit*/)
{
  // Sort list by priority
  std::sort(mPairList.begin(), mPairList.end(), ComparePairByPriority);

  // Replace server reflexive candidates with bases
  for (unsigned ci=0; ci<mPairList.size(); ci++)
  {
    PCandidatePair& pair = mPairList[ci];
    if (pair->first().mType == Candidate::ServerReflexive)
      pair->first().mType = Candidate::Host;
  }
  
  /*
#ifdef _WIN32
  Win32Preprocess();
#endif
  */

  pruneDuplicates();
  
  // Erase all relayed checks to LAN candidates
  PairList::iterator pairIter = mPairList.begin();
  while (pairIter != mPairList.end())
  {
    PCandidatePair& pair = *pairIter;
    if (pair->first().mType == Candidate::ServerRelayed && !pair->second().mExternalAddr.isPublic())
      pairIter = mPairList.erase(pairIter);
    else
      pairIter++;
  }

#ifndef ICE_LOOPBACK_SUPPORT
  pairIter = mPairList.begin();
  while (pairIter != mPairList.end())
  {
    PCandidatePair& pair = *pairIter;
    if (pair->second().mExternalAddr.isLoopback())
      pairIter = mPairList.erase(pairIter);
    else
      pairIter++;
  }
#endif

#ifdef ICE_SKIP_LINKLOCAL
  pairIter = mPairList.begin();
  while (pairIter != mPairList.end())
  {
    PCandidatePair& pair = *pairIter;
    if (pair->second().mExternalAddr.isLinkLocal())
      pairIter = mPairList.erase(pairIter);
    else
      pairIter++;
  }
#endif
  pruneDuplicates();
  
#ifdef ICE_POSTPONE_RELAYEDCHECKS
  postponeRelayed();
#endif

  // Put all LAN checks before other.
  // Therefore it should be priorities sorting should be enough.
  // But in answer's SDP remote peer can put external IP to the top of the list
  // It can cause its promote to the top of list of connectivity checks
  std::sort(mPairList.begin(), mPairList.end(), [](const PCandidatePair& p1, const PCandidatePair& p2) -> bool
  {
    return p1->isLanOnly() && !p2->isLanOnly();
  });

  // Cut all checks that are behind the limit
  if (mPairList.size() > ICE_CONNCHECK_LIMIT)
  {
    ICELogDebug(<<"Cut extra connection checks. The total number of checks should not exceed " << ICE_CONNCHECK_LIMIT);
    PairList::iterator bcIter = mPairList.begin();
    std::advance(bcIter, ICE_CONNCHECK_LIMIT);
    mPairList.erase(bcIter, mPairList.end());
  }
}

void CheckList::win32Preprocess()
{
  for (unsigned i=0; i<mPairList.size(); i++)
  {
    PCandidatePair& pair = mPairList[i];
    ICELogDebug(<<"Win32Preprocess pair " << pair->toStdString());

    if (pair->first().mType == Candidate::Host)
    {
      // Get best source interface for remote candidate
      NetworkAddress bestInterface = NetworkHelper::instance().sourceInterface(pair->second().mExternalAddr);
      
      // Replace IP address to found
      if (!bestInterface.isEmpty())
        pair->first().mLocalAddr.setIp(bestInterface.ip());
      else
        ICELogDebug(<<"Failed to find source interface for remote candidate");
    }
  }
}

PCandidatePair CheckList::findEqualPair(CandidatePair& _pair, ComparisionType ct)
{
  for (PCandidatePair& p: mPairList)
  {
    if (p->role() != CandidatePair::None)
    {
      switch (ct)
      {
      case CT_TreatHostAsUniform:
        if (p->first().mType == Candidate::Host && _pair.first().mType == Candidate::Host && p->second() == _pair.second())
          return p;
        if (p->first().mLocalAddr == _pair.first().mLocalAddr && p->first().mType == Candidate::Host && p->second() == _pair.second() && _pair.first().mType != Candidate::ServerRelayed)
          return p;
        if (_pair == *p)
          return p;
          
        break;

      case CT_Strict:
        if (_pair == *p)
          return p;
        break;
      }
    }
  }

  return PCandidatePair();
}

unsigned CheckList::add(CandidatePair& p)
{
  mPairList.push_back(PCandidatePair(new CandidatePair(p)));
  
  // Sort list by priority
  std::sort(mPairList.begin(), mPairList.end(), ComparePairByPriority);
  
  return (unsigned)mPairList.size();
}

unsigned CheckList::count()
{
  return (unsigned)mPairList.size();
}
std::string CheckList::toStdString()
{
  std::string dump = "";
  for (size_t i=0; i<mPairList.size(); i++)
  {
    dump += mPairList[i]->toStdString();
    dump += "\n";
  }

  return dump;
}

PCandidatePair& CheckList::operator[] (size_t index)
{
  return mPairList[index];
}

PCandidatePair CheckList::findNominatedPair(int component)
{
  for (PCandidatePair& p: mPairList)
  {
    if (p->first().component() == component &&
        p->role() != CandidatePair::None &&
        p->nomination() == CandidatePair::Nomination_Finished)
      return p;
  }
  return PCandidatePair();
}

PCandidatePair CheckList::findValidPair(int component)
{
  for (PCandidatePair& p: mPairList)
  {
    if (p->first().component() == component &&
        p->role() == CandidatePair::Valid)
      return p;
  }
  return PCandidatePair();
}

PCandidatePair CheckList::findBestValidPair(int componentId)
{
  PairList found;
  std::copy_if(mPairList.begin(), mPairList.end(), std::back_inserter(found),
               [componentId](const PCandidatePair& p) -> bool
  {
    return (p->first().component() == componentId &&
        p->role() == CandidatePair::Valid &&
        p->first().mExternalAddr.isLAN() &&
        p->second().mExternalAddr.isLAN());
  });

  if (found.size())
    return found.front();

  return findValidPair(componentId);
}

PCandidatePair CheckList::findHighestNominatedPair(int component)
{
#ifdef ICE_POSTPONE_RELAYEDCHECKS
  PairList found;
  for (PCandidatePair& p: mPairList)
  {
    if (p->first().component() == component &&
        p->role() != CandidatePair::None &&
        p->nomination() == CandidatePair::Nomination_Finished &&
        p->first().mType != Candidate::ServerRelayed &&
        p->second().mType != Candidate::ServerRelayed)
      found.push_back( p );
  }

  if (found.size())
  {
    PCandidatePair& f0 = found.front();
    for (PCandidatePair& p: mPairList)
      if (p == f0)
        return p;
  }
#endif
  int64_t priority = -1;
  PCandidatePair result;
  
  std::ostringstream oss;
  oss << "Looking for highest nominated pair in list:";
  for (PCandidatePair& p: mPairList)
  {
    oss << "\n  " << p->toStdString().c_str() << ", priority " << p->priority();
    
    if (p->first().component() == component &&
        p->role() != CandidatePair::None &&
        p->nomination() == CandidatePair::Nomination_Finished &&
        p->priority() > priority )
    {
      result = p;
      priority = p->priority();
    }
  }

  // ICELogDebug( << oss.str() );

  if (result)
  {
    ICELogDebug(<< "Result is " << result->toStdString());
  }
  else
  {
    // ICELogDebug(<< "No nominated pair");
  }
  return result;
}

void CheckList::removePairs(CandidatePair::State state, int component)
{
  for (unsigned i=0; i<mPairList.size(); i++)
  {
    CandidatePair& p = *mPairList[i];
    if (p.first().component() == component && 
        p.state() == state &&
        p.role() != CandidatePair::None)
      p.setRole(CandidatePair::None);
  }
}

PCandidatePair CheckList::findLowestNominatedPair(int component)
{
#ifdef _MSC_VER
  int64_t priority = 0x7FFFFFFFFFFF;
#else
  int64_t priority = 0x7FFFFFFFFFFFLL;
#endif
  PCandidatePair result;

  for (PCandidatePair& p: mPairList)
  {
    if (p->first().component() == component &&
        p->role() != CandidatePair::None &&
        p->nomination() == CandidatePair::Nomination_Finished &&
        p->priority() < priority )
    {
      result = p;
      priority = p->priority();
    }
  }

  return result;
}

void CheckList::updatePairPriorities()
{
  for (unsigned i=0; i<mPairList.size(); i++)
    mPairList[i]->updatePriority();
}

void CheckList::clear()
{
  mState = Running;
  mPairList.clear();
}

void CheckList::dump(std::ostream& output)
{
  output << Logger::TabPrefix << Logger::TabPrefix << "State: " << CheckList::stateToString(mState) << std::endl;
  for (unsigned i=0; i<mPairList.size(); i++)
    if (mPairList[i]->role() != CandidatePair::None)
      output << Logger::TabPrefix << Logger::TabPrefix << mPairList[i]->toStdString().c_str() << std::endl;
}

unsigned CheckList::countOfValidPairs()
{
  unsigned result = 0;
  for (unsigned i=0; i<mPairList.size(); i++)
    if (mPairList[i]->role() >= CandidatePair::Valid)
      result++;
  return result;
}

void CheckList::postponeRelayed()
{
  PairList relayed, relayedTarget;

  PairList::iterator pairIter = mPairList.begin();
  while ( pairIter != mPairList.end() )
  {
    PCandidatePair& p = *pairIter;
    if (p->first().mType == Candidate::ServerRelayed)
    {
      relayed.push_back(p);
      pairIter = mPairList.erase( pairIter );
    }
    else
    if (p->second().mType == Candidate::ServerRelayed)
    {
      relayedTarget.push_back(p);
      pairIter = mPairList.erase( pairIter );
    }
    else
      pairIter++;
  }
  
  for (pairIter = relayedTarget.begin(); pairIter != relayedTarget.end(); pairIter++)
    mPairList.push_back(*pairIter);
  
  for (pairIter = relayed.begin(); pairIter != relayed.end(); pairIter++)
    mPairList.push_back(*pairIter);
}
