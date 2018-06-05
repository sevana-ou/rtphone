/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_CANDIDATE_PAIR_H
#define __ICE_CANDIDATE_PAIR_H

#include "ICEPlatform.h"
#include "ICECandidate.h"
#include "ICEByteBuffer.h"
#include "ICESmartPtr.h"

namespace ice
{
  class CandidatePair
  {
  public:
    /// Possible pair states
    enum State
    {
      Waiting = 0,
      InProgress,
      Succeeded,
      Failed,
      Frozen
    };
  
    enum Role
    {
      None = 0,
      Regular = 1,
      Triggered = 2,
      Valid = 3
    };
		
    enum Nomination
    {
      Nomination_None,
      Nomination_Started,
      Nomination_Finished
    };
	  
    static const char* stateToString(State s);
	  static const char* nominationToString(Nomination n);
		
    
  protected:
    /// Local and remote candidates. First (zero index) is local candidate, second (index one) is remote candidate
    Candidate  mCandidate[2];
    
    /// Pair's priority, computed in UpdatePriority() method
    int64_t       mPriority;
    
    /// Pair's state
    State         mState;
    
    /// Index of controlled candidate in mCandidate array
    int           mControlledIndex;
    
    /// Index of controlling candidate in mCandidate array
    int           mControllingIndex;
    
    /// Combination of controlled and controlling candidates foundations, computed in UpdateFoundation() method.
    char          mFoundation[65];
    
    /// Marks nominated pair
    Nomination    mNomination;
  
    /// Mark pair role - regular, triggered, valid, nominated
    Role          mRole;
    void*         mTransaction;

    static const char*   roleToString(Role r);

  public:
    CandidatePair();
    CandidatePair(const CandidatePair& src);
    ~CandidatePair();

    Candidate& first();
    const Candidate& first() const;
    Candidate& second();
    const Candidate& second() const;

    int64_t       priority() const;
    void          setPriority(int64_t priority);
    State         state() const;
    void          setState(State state);
    const char*   foundation() const;
    void          setFoundation(const char* foundation);
    Role          role() const;
    void          setRole(Role role);
    Nomination    nomination() const;
    void          setNomination(Nomination n);
    unsigned      controlledIndex();
    void          setControlledIndex(unsigned index);
    unsigned      controllingIndex();
    void          setControllingIndex(unsigned index);
    void          updatePriority();
    void          updateFoundation();
    std::string   toStdString();
    bool          isLanOnly() const;

    void*         transaction();
    void          setTransaction(void* t);
    CandidatePair& operator = (const CandidatePair& rhs);
		bool operator == (const CandidatePair& rhs) const;
  };
  
  typedef std::shared_ptr<CandidatePair> PCandidatePair;
}
#endif
