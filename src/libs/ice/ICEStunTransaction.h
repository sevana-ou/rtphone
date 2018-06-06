/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_STUN_TRANSACTION_H
#define __ICE_STUN_TRANSACTION_H

#include <string>
#include <deque>

#include "ICEStunMessage.h"
#include "ICEPacketTimer.h"
#include "ICECandidatePair.h"
#include "ICEAction.h"

namespace ice
{
  class Logger;

  class Transaction
  {
  public:
    enum Type
    {
      Binding   = 1,
      Relaying  = 2,
      KeepAlive = 4,
      All = 7,
      None = 8,
    };

    enum State
    {
      Running = 0,      //Transaction is running now
      Failed = 1,       //Transaction is failed
      Success = 2       //Transaction is succeeded
    };
    static const char* stateToString(State state);

    Transaction();
    virtual ~Transaction();
    
    void generateId();
    
    // Returns if transaction is relayed and must be sent to TURN server instead its regular destination
    bool relayed();

    // Marks if transaction works through relay
    void setRelayed(bool relayed);
    
    // Attempts to generate outgoing data.
    // It is time-sensitive method - it can return empty pointer if there is not time to send data to network.
    virtual ByteBuffer* 
                    generateData(bool force = false);
    
    // Process incoming STUN message with source address
    virtual bool    processData(StunMessage& msg, NetworkAddress& address) = 0;

    // Attempts to restart transaction
    virtual void    restart();

    // Checks if transaction is timeouted.
    bool            isTimeout();
    
    // Adds MESSAGE-INTEGRITY attribute to outgoing messages
    void            addMessageIntegrity(bool enable);
    
    // Adds FINGERPRINT attribute to outgoing messages
    void            addFingerprint(bool enable);
    
    // Enables shortterm credentials on this transaction
    void            addShortTermCredentials(bool enable);
    
    // Enables longterm credentials on this transaction
    void            addLongTermCredentials(bool enable);
    
    // Adds CONTROLLING attribute with specified tie breaker value
    void            addControllingRole(const std::string& tieBreaker);
    
    // Adds CONTROLLED attribute with specified tie breaker value
    void            addControlledRole(const std::string& tieBreaker);
    
    // Adds PRIORITY attribute with specified value
    void            addPriority(unsigned int priority);
    
    // Returns received PRIORITY attribute
    unsigned int    priorityValue();

    void            setUsername(const std::string& username);
    void            setPassword(const std::string& password);
    
    // Sets associated tag value
    void            setTag(unsigned int tag);
    unsigned int    tag();
    
    // Returns transaction state - Succeeded, Failed, Running
    State           state() const;

    // Associates action to be performed on finish
    void            setAction(PAction action);
    PAction         action() const;
    
    // Ceases all outgoing transmission for this transaction
    void            cancel();
    bool            isCancelled() const;

    void            setComment(const std::string& comment);
    std::string     comment() const;
    
    void            setDestination(const NetworkAddress& addr);
    NetworkAddress&     destination();

    void            setComponent(int component);
    int             component();

    void            setStackId(int id);
    int             stackId();

    void            setTransportType(int _type);
    int             transportType();
    
    bool            keepalive();
    void            setKeepalive(bool keepalive);
		
    // Interval for keepalive transactions in seconds
    unsigned        interval();
    void            setInterval(unsigned interval);
    
    unsigned        timestamp();
    void            setTimestamp(unsigned timestamp);

    void*           userObject();
    void            setUserObject(void* obj);
    
    Type            type();
    void            setType(Type t);
		
		bool						removed();
		void						setRemoved(bool removed);
    
    virtual bool    hasToRunNow();

    StunMessage::TransactionID transactionId();

    std::string     toStdString();
    
    void            setFailoverId(int failoverId);
    int             failoverId() const;
    
  protected:
    
    // Logger
    Logger*      mLog;
    
    // Retransmission timer
    PacketScheduler  mRTOTimer;
    
    // Marks if transaction is cancelled
    bool            mCancelled;

    // Marks if packet was created
    bool            mComposed;

    // Long-term credentials
    std::string     mUsername;
    std::string     mPassword;
    
    // Marks if long term credentials must be used
    bool            mLongTerm;
    
    // Marks if short term credentials must be used
    bool            mShortTerm;
  
    // Marks if CONTROLLING attribute must be used
    bool            mEnableControlling;

    // Marks if CONTROLLED attribute must be used
    bool            mEnableControlled;
    
    // Marks if PRIORITY attribute must be used
    bool            mEnablePriority;
    
    // Priority value
    unsigned int    mPriority;

    // Tie breaker value for CONTROLLING&CONTROLLED attributes
    std::string     mTieBreaker;

    // Transaction ID
    StunMessage::TransactionID mTransactionID;
    
    // Buffer to hold raw outgoing data
    ByteBuffer         mOutgoingData;

    // Marks if fingerprint attribute must be used for outgoing messages
    bool                  mFingerprint;
    
    // Marks if message integrity attribyte must be used for outgoing messages
    bool                  mMessageIntegrity;
    
    // Tag associated with transaction
    unsigned int          mTag;
    
    // Transaction state - Running, Failed or Success
    State                 mState;
    
    // Action that must be run on transaction finish
    PAction               mAction;

    // Comment that describes this transaction
    std::string           mComment;
    
    // Destination address
    NetworkAddress        mDestination;

    // Relayed flag
    bool                  mRelayed;

    // Source component's ID
    int                   mComponent;
    
    // Used ICE stack ID. This member is used for debugging purpose.
    int                   mStackID;
    int                   mTransportType;
    
    bool                  mKeepalive;
    unsigned              mInterval;
    void*                 mUserObject;
    Type                  mType;
		bool									mRemoved;
    int                   mFailoverId;
    
    // Timestamp when transaction runs in last time. Default is zero.
    unsigned              mTimestamp;

    // Updates msg with auth. data (MessageDigest + Fingerprint) + fills mOutgoingData
    void                  enqueueMessage(ice::StunMessage& msg);


  };
}

#endif
