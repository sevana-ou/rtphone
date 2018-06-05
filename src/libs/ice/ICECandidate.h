/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_CANDIDATE_H
#define __ICE_CANDIDATE_H

#include "ICEPlatform.h"
#include "ICESmartPtr.h"
#include "ICEAddress.h"

#include <string>

namespace ice 
{
  struct Candidate
  {
    enum Type
    {
      Host = 0,
      ServerReflexive = 1,
      PeerReflexive = 2,
      ServerRelayed = 3
    };
    
    // Type of candidate - host, server reflexive/relayes or peer reflexive
    Type            mType;
    
    // External address
    NetworkAddress      mExternalAddr;

    // The component ID in ICE session
    int             mComponentId;

    // Mark the gathered candidate
    bool            mReady;
    
    // Mark the failed candidate - gathering is failed
    bool            mFailed;
    
    // The candidate priority
    unsigned int    mPriority;
    
    // Candidate's foundation
    char            mFoundation[33];
    
    // Interface priority
    unsigned int    mInterfacePriority;

    // Local used address
    NetworkAddress      mLocalAddr;

    Candidate()
      :mType(Host), mComponentId(0), mReady(false), mFailed(false), mPriority(0),
      mInterfacePriority(0)
    {
      memset(mFoundation, 0, sizeof mFoundation);
    }
      
    /*  Constructor.
     *  @param _type Type of candidate - host, reflexive or relayed.
     *  @param componentID ID of component where candidate is used.
     *  @param portNumber Local port number.
     *  @param ipAddress Local IP address.
     *  @param interfacePriority Priority of specified interface.
     *  @param startTimeout Start time for STUN/TURN checks. */
    Candidate(Type _type)
      : mType(_type), mComponentId(0), mReady(false), mFailed(false), mPriority(0),
      mInterfacePriority(0)
    {
      // Check if type is "host" - the candidate is ready in this case
      if (_type == Host)
        mReady = true;
      memset(mFoundation, 0, sizeof mFoundation);
    }
    
    ~Candidate()
    {}
    
    // Sets local and external address simultaneously
    void                  setLocalAndExternalAddresses(std::string& ip, unsigned short portNumber);
    void                  setLocalAndExternalAddresses(NetworkAddress& addr);
    void                  setLocalAndExternalAddresses(NetworkAddress &addr, unsigned short altPort);

    // Returns type of candidate as string - 'host', 'reflexive' or 'relayed'
    const char*           type();
    
    // Computes priority value basing on members and type priority list
    void                  computePriority(int* typepreflist);
    
    // Updates foundation value depending on members
    void                  computeFoundation();
 
    // Returns SDP line for this candidate
    std::string           createSdp();

    // Creates ICECandidate instance based on SDP line
    static Candidate   parseSdp(const char* sdp);

    // Returns candidate type basing on string type representation
    static Type           typeFromString(const char* candtype);

    // Compares if two candidates are equal
    static bool           equal(Candidate& cand1, Candidate& cand2);

    bool operator ==  (const Candidate& rhs) const;
    void dump(std::ostream& output);
    int component();
  };

};
#endif
