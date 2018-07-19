/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEAuthTransaction.h"
#include "ICEStunAttributes.h"
#include "ICEMD5.h"
#include "ICELog.h"

using namespace ice;

#define LOG_SUBSYSTEM "ICE"

AuthTransaction::AuthTransaction()
:Transaction(), mActive(false), mComposed(false), mConformsToKeepaliveSchedule(true),
mCredentialsEncoded(false)
{
}

AuthTransaction::~AuthTransaction()
{
}

void AuthTransaction::init()
{
  mConformsToKeepaliveSchedule = false;
  if (mRealm.size() && mNonce.size())
    buildAuthenticatedMsg();
  else
  {
    std::shared_ptr<StunMessage> msg(new StunMessage());
    msg->setTransactionId(mTransactionID);
    setInitialRequest(*msg);
    mComposed = false;
    mOutgoingMsgQueue.push_back(msg);
  }
}

void AuthTransaction::buildAuthenticatedMsg()
{
  // Setup key for MessageIntegrity
  std::string key = mUsername + ":" + mRealm + ":" + mPassword;
  md5Bin(key.c_str(), key.size(), mKey);

  // Create new authenticated message
  std::shared_ptr<StunMessage> newMsg( new StunMessage() );

  // Optional - generate new transaction ID
  // mTransactionID = StunMessage::TransactionID::GenerateNew();

  newMsg->setTransactionId( mTransactionID );

  // Add USERNAME
  newMsg->usernameAttr().setValue( mUsername );
  newMsg->realmAttr().setValue( mRealm );
  newMsg->nonceAttr().setValue( mNonce );      

  // Adjust message
  setAuthenticatedRequest( *newMsg );

  // Ensure MessageIntegrity exists
  newMsg->messageIntegrityAttr().setValue( NULL );

  // Enqueue msg
  mComposed = false;

  mOutgoingMsgQueue.clear();
  mOutgoingMsgQueue.push_back( newMsg );
}

bool AuthTransaction::processData(StunMessage& msg, NetworkAddress& address)
{
  if (msg.transactionId() != mTransactionID)
    return false;
  
  // Check for 401 error code
  if (msg.hasAttribute(StunAttribute::ErrorCode))
  {
    ErrorCode& ec = msg.errorCodeAttr();
    
    // Get realm value - it must be long term credentials
    if (ec.errorCode() == 401 && (!mCredentialsEncoded || !mLongTerm))
    {
      if (!msg.hasAttribute(StunAttribute::Realm) || !msg.hasAttribute(StunAttribute::Nonce))
        return false;
      
      Realm& realm = msg.realmAttr();
      
      // Extract realm and nonce 
      mRealm = realm.value();
      mNonce = msg.nonceAttr().value();
      
      // Change transaction id
      if (mLongTerm)
      {
        mCredentialsEncoded = true;
        generateId();
      }
      ICELogDebug(<< "Server requested long term credentials for realm " << mRealm);
      buildAuthenticatedMsg();
    }
    else
    if (ec.errorCode() == 438 && (msg.hasAttribute(StunAttribute::Realm) || msg.hasAttribute(StunAttribute::Nonce)))
    {
      if (msg.hasAttribute(StunAttribute::Realm))
        mRealm = msg.realmAttr().value();
      if (msg.hasAttribute(StunAttribute::Nonce))
        mNonce = msg.nonceAttr().value();
      
      ICELogDebug(<< "Returned error 438, using new nonce");
      if (msg.hasAttribute(StunAttribute::Realm) && msg.hasAttribute(StunAttribute::Nonce))
      {
        if (mLongTerm)
        {
          mCredentialsEncoded = true;
          generateId();
        }
      }
      buildAuthenticatedMsg();
    }  
    else
    {
      mErrorCode = ec.errorCode();
      mErrorResponse = ec.errorPhrase();
      mState = Transaction::Failed;
      processError();
      
      ICELogError(<<"Stack ID " << mStackID << ". Got error code " << mErrorCode << " for STUN transaction. Error message: " << ec.errorPhrase());
    }
  }
  else
  if (msg.messageClass() == StunMessage::ErrorClass)
  {
    mErrorCode = 0;
    mState = Transaction::Failed;
    processError();
    ICELogError(<<"Stack ID " << mStackID << ". Got ErrorClass response.");
  }
  else
  {
    ICELogDebug(<< "Process STUN success message");
    processSuccessMessage(msg, address);
    mState = Transaction::Success;
  }

  return true;
}

ByteBuffer* AuthTransaction::generateData(bool force)
{
  if (!mActive)
  {
    init();
    mActive = true;
  }

  if (!mComposed)
  {
    // Restart retransmission timer as new message will be built
    mRTOTimer.stop();
    mRTOTimer.start();
    
    // Get message from outgoing queue
    if (!mOutgoingMsgQueue.empty())
    {
      // Clear buffer for raw data
      mOutgoingData.clear();

      // Encode next message
      StunMessage& msg = *mOutgoingMsgQueue.front();
      
      //ICELogDebug(<< "Stack ID " << mStackID << ". Build message " << msg.GetTransactionID().ToString() << " from transaction " << this->GetComment());
      
      if (mShortTerm)
        msg.buildPacket(mOutgoingData, mPassword);
      else
        msg.buildPacket(mOutgoingData, std::string((char*)mKey, 16));
      
      // Remove encoded message from msg queue
      mOutgoingMsgQueue.pop_front();
    }
    else
      ICELogDebug(<< "No outgoing message in queue");
    
    mComposed = true;
  }

  return Transaction::generateData(force);
}

int AuthTransaction::errorCode()
{
  return mErrorCode;
}

std::string AuthTransaction::errorResponse()
{
  return mErrorResponse;
}

void AuthTransaction::restart()
{
  // Clear outgoing data
  mOutgoingData.clear();
  mOutgoingMsgQueue.clear();

  mState = Transaction::Running;

  // Mark "message is not built yet"
  mComposed = false;

  // Mark "first request is not sent yet"
  mActive = false;

  // Reset retransmission timer
  mRTOTimer.stop();
  mRTOTimer.start();
  mCredentialsEncoded = false;
  mConformsToKeepaliveSchedule = true;
}

bool AuthTransaction::hasToRunNow()
{
  bool result = Transaction::hasToRunNow();
  if (!result && keepalive())
  {
    result |= mState == /*TransactionState::*/Running && !mConformsToKeepaliveSchedule;
  }
  return result;
}

std::string AuthTransaction::realm() const
{
  return mRealm;
}

void AuthTransaction::setRealm(std::string realm)
{
  mRealm = realm;
}

std::string AuthTransaction::nonce() const
{
  return mNonce;
}

void AuthTransaction::setNonce(std::string nonce)
{
  mNonce = nonce;
}
