/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_BINDING_H
#define __ICE_BINDING_H

#include "ICEStunTransaction.h"
#include "ICEAuthTransaction.h"
#include "ICEAddress.h"

namespace ice
{
  class CheckResult
  {
  public:
    virtual Transaction::State resultState() = 0;
    virtual int resultError() = 0;
    virtual NetworkAddress& resultSource() = 0;
    virtual NetworkAddress& resultLocal() = 0;
    virtual void useCandidate() = 0;
    virtual unsigned resultPriority() = 0;
  };

  class ConnectivityCheck: public Transaction, public CheckResult
  {
  public:
    ConnectivityCheck();
    virtual ~ConnectivityCheck();

    ByteBuffer*  generateData(bool force = false);
    bool            processData(StunMessage& msg, NetworkAddress& sourceAddress);

    NetworkAddress&     mappedAddress();       //returns result from succesful transaction
    NetworkAddress&     responseAddress();     //returns responses source address
    int             errorCode();
    void            addUseCandidate();
    void            confirmTransaction(NetworkAddress& mapped);

    // CheckResult interface
    int resultError();
    NetworkAddress& resultSource();
    NetworkAddress& resultLocal();
    State resultState();
    void useCandidate();
    unsigned resultPriority();
  protected:
    NetworkAddress      mResponseAddress;
    NetworkAddress      mMappedAddress;
    bool            mUseCandidate;
    int             mErrorCode;
    std::string     mErrorResponse;
  };

  class ClientBinding: public Transaction
  {
  public:
    ClientBinding();
    virtual ~ClientBinding();
    
    int             errorCode();           //returns error code from failed transaction
    std::string     errorResponse();       //returns error msg from failed transaction
    NetworkAddress&     mappedAddress();       //returns result from succesful transaction
    NetworkAddress&     responseAddress();     //returns responses source address
    
    bool              processData(StunMessage& msg, NetworkAddress& address);
    ByteBuffer*    generateData(bool force = false);
    void              addUseCandidate();

  protected:
    int             mErrorCode;
    NetworkAddress      mMappedAddress;
    std::string     mErrorResponse;
    NetworkAddress      mResponseAddress;
    bool            mUseCandidate;
  };

  class ServerBinding: public Transaction
  {
  public:
    ServerBinding();
    virtual ~ServerBinding();
    void setLocalTieBreaker(std::string tieBreaker);

    bool              processData(StunMessage& msg, NetworkAddress& address);
    ByteBuffer*    generateData(bool force = false);
    void              restart();
    
    // Checks if incoming request  has UseCandidate attribute
    bool                      hasUseCandidate();
    
    // Checks if processed StunMessage was authenticated ok
    bool                      gotRequest();
    
    // Gets the role from processed message. It can be Controlled or Controlling or None.
    int                       role();
    
    // Instructs transaction to response with 487 code
    void                      generate487();
    
    // Returns received tie breaker
    std::string               remoteTieBreaker();

  protected:
    NetworkAddress        mSourceAddress;   
    bool              mUseCandidate;    
    bool              mGenerate400;
    bool              mGenerate487;
    int               mRole;
    std::string       mLocalTieBreaker;
    std::string       mRemoteTieBreaker;
  };


  class BindingIndication: public Transaction
  {
  public:
    BindingIndication(unsigned int interval);
    virtual ~BindingIndication();

    bool              processData(StunMessage& msg, NetworkAddress& address);
    ByteBuffer*    generateData(bool force = false);

  protected:
    unsigned mTimestamp;
    unsigned mInterval;
  };
}

#endif