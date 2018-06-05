/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICEStunTransaction.h"
#include "ICEStunAttributes.h"
#include "ICELog.h"
#include "ICEError.h"
#include "ICEAction.h"
#include "ICETime.h"

#ifndef _WIN32
# include <stdexcept>
#endif

using namespace ice;
#define LOG_SUBSYSTEM "ICE"

const char* Transaction::stateToString(State state)
{
  switch (state)
  {
  case Running: return "Running";
  case Failed:  return "Failed";
  case Success: return "Success";
  }
  return "Undefined";
}

Transaction::Transaction()
{
  mTransportType = IPv4;

  // There is no default ICE stack
  mStackID = 0;

  // There is no default logger
  mLog = NULL;

  // Packet is not composed in constructor
  mComposed = false;

  // Initial state of transaction - "Running"
  mState = Transaction::Running;

  // Reset user data
  mTag = 0;

  // Do not use message integrity attribute by default
  mMessageIntegrity = false;

  // Do not use fingerprint attribute by default
  mFingerprint = false;

  // Do not use long term credentials by default
  mLongTerm = false;

  // Do not use short term credentials by default
  mShortTerm = false;

  // Create new transaction ID
  generateId();
  
  // CONTROLLING attribute is not used by default
  mEnableControlling = false;

  // CONTROLLED attribute is not used by default
  mEnableControlled = false;

  // PRIORITY attribute is not used by default
  mEnablePriority = false;
  mPriority = 0;

  // Transaction is not cancelled
  mCancelled = false;

  // Transaction is not relayed by default
  mRelayed = false;

  // Component port is not set by default
  mComponent = 0;

  mKeepalive = false;
  mInterval = 0;
  mUserObject = NULL;
  mType = None;
	mRemoved = false;
  mFailoverId = 0;
}

Transaction::~Transaction()
{
}

void Transaction::generateId()
{
  for (size_t i=0; i<12; i++)
    mTransactionID.mValue[i] = rand();
}
void Transaction::setStackId(int id)
{
  mStackID = id;
}

int Transaction::stackId()
{
  return mStackID;
}

void Transaction::setTransportType(int _type)
{
  assert(_type == IPv4 || _type == IPv6);
  mTransportType = _type;
}

int Transaction::transportType()
{
  return mTransportType;
}

std::string Transaction::toStdString()
{
  std::ostringstream output;
  output << "State: " << Transaction::stateToString(mState) << ", ";
  output << "Transport: " << (mTransportType == IPv4 ? "IPv4" : "IPv6") << ", ";
  output << (mCancelled ? "Cancelled, " : "");
  output << "Destination: " << mDestination.toStdString().c_str() << ", ";
  output << "Comment: " << mComment.c_str();

  return output.str();
}

void Transaction::setTag(unsigned int tag)
{
  mTag = tag;
}

unsigned Transaction::tag()
{
  return mTag;
}

void Transaction::enqueueMessage(ice::StunMessage& msg)
{
  // Clear outgoing buffer
  mOutgoingData.clear();
  
  // Check if message should be secured by message integrity
  std::string password = mPassword;
  if (true == mMessageIntegrity)
  {
    // Ensure credentials are enabled
    assert(mLongTerm || mShortTerm);

    // Create username attribute
    Username* attr = new Username();
      
    // Set username value
    if (mLongTerm || mShortTerm)
      attr->setValue(mUsername);

    // Add username attribute itself
    msg.setAttribute(attr);

    // Add message integrity attribute
    msg.setAttribute(new MessageIntegrity());
  }

  if (mFingerprint)
    msg.setAttribute(new Fingerprint());
  
  // Add ICE-CONTROLLED attribute if needed
  if (mEnableControlled)
  {
    assert(mTieBreaker.length() == 8);

    ControlledAttr* attr = new ControlledAttr();
    attr->setTieBreaker(mTieBreaker);
    msg.setAttribute(attr);
  }

  // Add ICE-CONTROLLING attribute if needed
  if (mEnableControlling)
  {
    assert(mTieBreaker.length() == 8);
    
    ControllingAttr* attr = new ControllingAttr();
    attr->setTieBreaker(mTieBreaker);
    msg.setAttribute(attr);
  }

  // Add ICE-PRIORITY attribute if needed
  if (mEnablePriority)
  {
    ICEPriority* attr = new ICEPriority();
    attr->setPriority(mPriority);
    msg.setAttribute(attr);
  }

  msg.buildPacket(mOutgoingData, password);
}

ByteBuffer* Transaction::generateData(bool force)
{
  if (mCancelled)
    return NULL;

  // Check if there is any outgoing data
  if (mOutgoingData.size() == 0)
    return NULL;

  // Check if there is timeout for next transmission attempt
  if (mRTOTimer.isTimeout() && !force)
  {
    ICELogCritical(<< "Transaction " << mComment << " timeouted.");
    return NULL;
  }

  // Check if transmission was made too much times - client should not send billions of packets.
  if (mRTOTimer.isAttemptLimitReached() && !force)
    return NULL;

  // Check if time to retransmit now
  if (mRTOTimer.isTimeToRetransmit() || force)
  {
    mRTOTimer.attemptMade();

    // Copy outgoing data
    ByteBuffer* buffer = new ByteBuffer(mOutgoingData);
    
    buffer->setComment(mComment);
    buffer->setRemoteAddress(destination());
    buffer->setComponent(component());
    buffer->setComment(comment());
    buffer->setTag(this);
    return buffer;
  }
  else
  {
  }
  return NULL;
}

void Transaction::restart()
{
  mComposed = false;
  mState = Transaction::Running;
  mOutgoingData.clear();
  mRemoved = false;
  mRTOTimer.stop();
  mRTOTimer.start();
}

bool Transaction::isTimeout()
{
  if (mRTOTimer.isTimeout())
  {
    mState = Failed;
    return true;
  }
  else
    return false;
}

void Transaction::addMessageIntegrity(bool enable)
{
  mMessageIntegrity = enable;
}

void Transaction::addFingerprint(bool enable)
{
  mFingerprint = enable;
}

void Transaction::addShortTermCredentials(bool enable)
{
  this->mShortTerm = enable;
}

void Transaction::addLongTermCredentials(bool enable)
{
  this->mLongTerm = enable;
}

void Transaction::addControllingRole(const std::string& tieBreaker)
{
  mEnableControlling = true;
  mTieBreaker = tieBreaker;
}

void Transaction::addControlledRole(const std::string& tieBreaker)
{
  mEnableControlled = true;
  mTieBreaker = tieBreaker;
}

void Transaction::addPriority(unsigned int priority)
{
  mEnablePriority = true;
  mPriority = priority;
}

unsigned int Transaction::priorityValue()
{
  if (!mEnablePriority)
    throw Exception(NO_PRIORITY_ATTRIBUTE);

  return mPriority;
}


void Transaction::setUsername(const std::string& username)
{
  mUsername = username;
}

void Transaction::setPassword(const std::string& password)
{
  mPassword = password;
}

Transaction::State Transaction::state() const
{
  return mState;
}

void Transaction::setAction(PAction action)
{
  mAction = action;
}

PAction Transaction::action() const
{
  return mAction;
}

void Transaction::cancel()
{
  mCancelled = true;
}

bool Transaction::isCancelled() const
{
  return mCancelled;
}

void Transaction::setComment(const std::string& comment)
{
  mComment = comment;
}

std::string Transaction::comment() const
{
  return mComment;
}

void Transaction::setDestination(const NetworkAddress& addr)
{
  mDestination = addr;
}

void Transaction::setComponent(int component)
{
  mComponent = component;
}

int Transaction::component()
{
  return mComponent;
}

NetworkAddress& Transaction::destination()
{
  return mDestination;
}

bool Transaction::relayed()
{
  return mRelayed;
}

void Transaction::setRelayed(bool relayed)
{
  mRelayed = relayed;
}

bool Transaction::keepalive()
{
  return mKeepalive;
}

void Transaction::setKeepalive(bool keepalive)
{
  mKeepalive = keepalive;
}

unsigned Transaction::interval()
{
  return mInterval;
}

void Transaction::setInterval(unsigned interval)
{
  mInterval = interval;
}

void* Transaction::userObject()
{
  return mUserObject;
}

void Transaction::setUserObject(void* obj)
{
  mUserObject = obj;
}

Transaction::Type Transaction::type()
{
  return mType;
}

void Transaction::setType(Transaction::Type t)
{
  mType = t;
}

bool Transaction::removed()
{
	return mRemoved;
}

void Transaction::setRemoved(bool removed)
{
  if (!mRemoved && removed)
    ICELogDebug(<< "Transaction " << mComment << " removed.");

  mRemoved = removed;
}

unsigned Transaction::timestamp()
{
  return mTimestamp;
}

void Transaction::setTimestamp(unsigned timestamp)
{
  mTimestamp = timestamp;
}

StunMessage::TransactionID Transaction::transactionId()
{
  return mTransactionID;
}

bool Transaction::hasToRunNow()
{
  if (!mKeepalive)
    return true;
  
  //bool cf = (mComment == "ClientRefresh");
  
  unsigned current = ICETimeHelper::timestamp();
  /*if (cf)
  {
    ICELogDebug(<< "ClientRefresh interval: " << ICETimeHelper::findDelta(timestamp(), current) << ", interval " << interval()*1000);
  }*/
  
  if (timestamp())
  {
    if (ICETimeHelper::findDelta(timestamp(), current) < interval() * 1000)
      return false;
    
    setTimestamp(current);
    return true;
  }
  else
  {
    setTimestamp(current);
    return false;
  }
}

void Transaction::setFailoverId(int failoverId)
{
  mFailoverId = failoverId;
}

int Transaction::failoverId() const
{
  return mFailoverId;
}
