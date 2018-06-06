/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_CHECK_LIST_H
#define __ICE_CHECK_LIST_H

#include "ICEPlatform.h"
#include "ICECandidatePair.h"
#include <vector>

namespace ice
{
  class CheckList
  {
  public:
    enum State
    {
      Running = 0,
      Completed,
      Failed
    };
    static const char* stateToString(int state);
    typedef std::vector<PCandidatePair> PairList;
   
  protected:
    /// Check list state
    State mState;
    
    /// Vector of pairs
    PairList mPairList;
  
    void pruneDuplicates();
    void postponeRelayed();

  public:
    CheckList();  
    
    State state();
    void setState(State state);
    PairList& pairlist();

    /// Sorts check list, prunes from duplicate pairs and cuts to checkLimit number of elements.
    void prune(int checkLimit);
    
    /// Replaces local host candidates IP addresses with best source interface.
    void win32Preprocess();
    
    /// Finds&returns smart pointer to pair 
    enum ComparisionType
    {
      CT_TreatHostAsUniform,
      CT_Strict
    };
    PCandidatePair findEqualPair(CandidatePair& p, ComparisionType ct);
    
    /// Add&sort 
    unsigned add(CandidatePair& p);
    
    /// Returns number of pairs in list
    unsigned count();
    
    /// Returns items of list
    //ICECandidatePair operator[] (size_t index) const;
    PCandidatePair& operator[] (size_t index);

    /// Dumps check list to string
    std::string toStdString();
    
    /// Updates state of list based on items state
    void updateState();

    /// Find first nominated pair for specified component ID
    PCandidatePair   findNominatedPair(int componentID);

    /// Finds valid pair for specified component ID. It is used for valid lists only.
    PCandidatePair   findValidPair(int componentID);

    /// Finds best valid pair for specified component ID. 'Best' means to prefer 1) LAN addresses 2) reflexive addresses 3)relayed addresses is low priority
    PCandidatePair   findBestValidPair(int componentId);

    /// Finds nominated pair with highest priority and specified component ID
    PCandidatePair   findHighestNominatedPair(int componentID);

    /// Finds nominated pair with highest priority and specified component ID
    PCandidatePair   findLowestNominatedPair(int componentID);

    /// Removes from list pairs with specified state and component ID.
    void       removePairs(CandidatePair::State state, int componentID);

    /// Recomputes pair priorities
    void       updatePairPriorities();

    void       clear();
    void       dump(std::ostream& output);
    unsigned   countOfValidPairs();
  };

}
#endif
