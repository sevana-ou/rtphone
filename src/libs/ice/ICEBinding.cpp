/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEBinding.h"
#include "ICEStunAttributes.h"
#include "ICETime.h"
#include "ICELog.h"
#include <memory>

using namespace ice;
#define LOG_SUBSYSTEM "ICE"

//----------------------- AuthClientBinding -----------------

ConnectivityCheck::ConnectivityCheck()
{
  setComment("ConnectivityCheck");
  mErrorCode = 0;
}

ConnectivityCheck::~ConnectivityCheck()
{
}

ByteBuffer* ConnectivityCheck::generateData(bool force)
{
  if (!mComposed)
  {
    StunMessage msg;
    msg.setMessageClass(StunMessage::RequestClass);
    msg.setMessageType(StunMessage::Binding);
    msg.setTransactionId(mTransactionID);
    
    if (mUseCandidate)
      msg.setAttribute(new ICEUseCandidate());
    
    // Use message integrity attribute
    mMessageIntegrity = true;

    // Use fingerprint attribute
    mFingerprint = true;

    if (mEnablePriority)
      msg.icePriorityAttr().setPriority(mPriority);

    // Copy comment to message object
    msg.setComment(mComment);

    enqueueMessage(msg);

    mComposed = true;
  }

  return Transaction::generateData(force);
}

void ConnectivityCheck::confirmTransaction(NetworkAddress& mapped)
{
  // Save resolved address&ip
  mMappedAddress = mapped;

  // Mark transaction as succeeded
  mState = Transaction::Success;
  
  // Save source IP and port
  mResponseAddress = mapped;
}

bool ConnectivityCheck::processData(StunMessage& msg, NetworkAddress& address)
{
#ifdef ICE_TEST_VERYAGGRESSIVE
  return false;
#endif

  // Check if it is response
  if (msg.messageClass() != StunMessage::ErrorClass &&
      msg.messageClass() != StunMessage::SuccessClass)
    return false;
  
  if (msg.transactionId() != mTransactionID)
    return false;
  
  // Validate againgst password used to encrypt
  if (!msg.validatePacket(mPassword))
    return false;

  // Check for ErrorCode attribute
  if (msg.hasAttribute(StunAttribute::ErrorCode))
  {
    ErrorCode& ec = dynamic_cast<ErrorCode&>(msg.attribute(StunAttribute::ErrorCode));
    
    //save error code and response
    mErrorCode = ec.errorCode();
    mErrorResponse = ec.errorPhrase();
    
    //mark transaction as failed
    mState = Transaction::Failed;

    return true;
  }

  //check if received empty ErrorClass response for poor servers
  if (msg.messageClass() == StunMessage::ErrorClass)
  {
    //mark transaction as failed 
    mState = Transaction::Failed;
    
    return true;
  }

  //check for mapped address attribute
  if (msg.hasAttribute(StunAttribute::MappedAddress))
  {
    // Save resolved address&ip
    mMappedAddress  = msg.mappedAddressAttr().address();

    // Mark transaction as succeeded
    mState = Transaction::Success;

    // Save source IP and port
    mResponseAddress = address;
  }
  
  //check for xor'ed mapped address attribute
  if (msg.hasAttribute(StunAttribute::XorMappedAddress))
  {
    // Save resolved IP and port
    mMappedAddress = msg.xorMappedAddressAttr().address();
    
    // Mark transaction as succeeded
    mState = Transaction::Success;

    // Save source IP and port
    mResponseAddress = address;
  }
  
  return true;
}

NetworkAddress& ConnectivityCheck::mappedAddress()
{
  return mMappedAddress;
}
NetworkAddress& ConnectivityCheck::responseAddress()
{
  return mResponseAddress;
}
void ConnectivityCheck::addUseCandidate()
{
  mUseCandidate = true;
}
int ConnectivityCheck::errorCode()
{
  return mErrorCode;
}
int ConnectivityCheck::resultError()
{
  return mErrorCode;
}
NetworkAddress& ConnectivityCheck::resultSource()
{
  return mResponseAddress;
}
Transaction::State ConnectivityCheck::resultState()
{
  return state();
}
NetworkAddress& ConnectivityCheck::resultLocal()
{
  return mMappedAddress;
}
void ConnectivityCheck::useCandidate()
{
  addUseCandidate();
}
unsigned ConnectivityCheck::resultPriority()
{
  return priorityValue();
}
//---------------------------- ClientBindingTransaction ----------------------------

ClientBinding::ClientBinding()
:Transaction(), mErrorCode(0), mUseCandidate(false)
{
  setComment("ClientBinding");
}

ClientBinding::~ClientBinding()
{
}
    
int ClientBinding::errorCode()
{
  return mErrorCode;
}

std::string ClientBinding::errorResponse()
{
  return mErrorResponse;
}

NetworkAddress& ClientBinding::mappedAddress()
{
  return mMappedAddress;
}

NetworkAddress& ClientBinding::responseAddress()
{
  return mResponseAddress;
}

bool ClientBinding::processData(StunMessage& msg, NetworkAddress& address)
{
  // Check if it is response
  if (msg.messageClass() != StunMessage::ErrorClass &&
      msg.messageClass() != StunMessage::SuccessClass)
    return false;
  
  if (msg.transactionId() != this->mTransactionID)
    return false;
  
  // Check for ErrorCode attribute
  if (msg.hasAttribute(StunAttribute::ErrorCode))
  {
    // Save error code and response
    mErrorCode = msg.errorCodeAttr().errorCode();
    mErrorResponse = msg.errorCodeAttr().errorPhrase();
    
    // Mark transaction as failed
    mState = Transaction::Failed;

    return true;
  }

  // Check if received empty ErrorClass response for poor servers
  if (msg.messageClass() == StunMessage::ErrorClass)
  {
    // Mark transaction as failed 
    mState = Transaction::Failed;
    
    return true;
  }

  // Check for mapped address attribute
  if (msg.hasAttribute(StunAttribute::MappedAddress))
  {
    // Save resolved address&ip
    mMappedAddress = msg.mappedAddressAttr().address();

    // mMark transaction as succeeded
    mState = Transaction::Success;

    // Save source IP and port
    mResponseAddress = address;
  }
  
  //check for xor'ed mapped address attribute
  if (msg.hasAttribute(StunAttribute::XorMappedAddress))
  {
    // Save resolved IP and port
    mMappedAddress = msg.xorMappedAddressAttr().address();

    // Mark transaction as succeeded
    mState = Transaction::Success;

    // Save source IP and port
    mResponseAddress = address;
  }
  
  return true;
}

ByteBuffer* ClientBinding::generateData(bool force)
{
  if (!mComposed)
  {
    StunMessage msg;
    msg.setMessageType(ice::StunMessage::Binding);
    msg.setMessageClass(ice::StunMessage::RequestClass);
    msg.setTransactionId(mTransactionID);
    
    if (mUseCandidate)
      msg.setAttribute(new ICEUseCandidate());

    // Copy comment to message object
    msg.setComment(mComment);

    enqueueMessage(msg);
    
    mComposed = true;
  }

  return Transaction::generateData(force);
}

void ClientBinding::addUseCandidate()
{
  mUseCandidate = true;
}
  

//------------------------------ ServerBindingTransaction ---------------------------
ServerBinding::ServerBinding()
{
  setComment("ServerBinding");

  mGenerate400 = false;
  mGenerate487 = false;
  mRole = 0;
}

ServerBinding::~ServerBinding()
{
}
void ServerBinding::setLocalTieBreaker(std::string tieBreaker)
{
  mLocalTieBreaker = tieBreaker;
}

bool ServerBinding::processData(StunMessage& msg, NetworkAddress& address)
{
  if (msg.messageType() != StunMessage::Binding || msg.messageClass() != StunMessage::RequestClass)
    return false;
  ICELogDebug(<< "Received Binding request from " << address.toStdString().c_str());
  
  // Save visible address
  mSourceAddress = address;
    
  // Save transaction ID
  mTransactionID = msg.transactionId();
    
  // Save Priority value 
  if (msg.hasAttribute(StunAttribute::ICEPriority))
  {
    mEnablePriority = true;
    mPriority = msg.icePriorityAttr().priority();
  }

  // Save UseCandidate flag
  mUseCandidate = msg.hasAttribute(StunAttribute::ICEUseCandidate);

  // Check if auth credentials are needed
  mGenerate400 = false; //do not send 400 response by default
  mGenerate487 = false;
  
  if (!msg.hasAttribute(StunAttribute::Username) || !msg.hasAttribute(StunAttribute::MessageIntegrity))
  {
    ICELogError(<< "There is no Username or MessageIntegrity attributes in Binding required. Error 400 will be generated.");
    // Send 400 error
    mGenerate400 = true;

    return true;
  }

  // Check for role
  if (msg.hasAttribute(StunAttribute::ControlledAttr))
  {
    mRemoteTieBreaker = msg.iceControlledAttr().tieBreaker();
    mRole = 1; // Session::Controlled;
  }
  else
  if (msg.hasAttribute(StunAttribute::ControllingAttr))
  {
    mRemoteTieBreaker = msg.iceControllingAttr().tieBreaker();
    mRole = 2;// Session::Controlling;
  }

  return true;
}


ByteBuffer* ServerBinding::generateData(bool force)
{
  // Restart timer
  mRTOTimer.stop();
  mRTOTimer.start();
  
  // See if remote password / username are set
  if (mPassword.empty() || mUsername.empty())
    return NULL;

  StunMessage msg;
  
  // Set transaction ID the same as processed incoming message
  msg.setTransactionId(mTransactionID);
  msg.setMessageType(StunMessage::Binding);
  
  if (mGenerate400)
  {
    // Generate bad request error
    msg.setMessageClass(StunMessage::ErrorClass);
    
    msg.errorCodeAttr().setErrorCode(400);
    msg.errorCodeAttr().setErrorPhrase("Bad request");
  }
  else
  if (mGenerate487)
  {
    // Generate 487 error
    msg.setMessageClass(StunMessage::ErrorClass);
    msg.errorCodeAttr().setErrorCode(487);
    msg.errorCodeAttr().setErrorPhrase("Role conflict");
  }
  else
  {
    msg.setMessageClass(StunMessage::SuccessClass);
    
    msg.mappedAddressAttr().address() = mSourceAddress;
    msg.xorMappedAddressAttr().address() = mSourceAddress;
  }

  // Build message
    
  // Clear outgoing buffer
  mOutgoingData.clear();
  
  // Check if message should be secured by message integrity
  //std::string password;
  if (!mGenerate400 && !mGenerate487)
  {
    // Do not create username attribute here - response does not need it
    // msg.usernameAttr().setValue(mUsername);
    
    //ICELogMedia(<< "Using password " << mPassword);

    // Add message integrity attribute
    msg.setAttribute(new MessageIntegrity());
    
    if (mFingerprint)
      msg.setAttribute(new Fingerprint());
  
    // Add ICE-CONTROLLED attribute if needed
    if (mEnableControlled)
      msg.iceControlledAttr().setTieBreaker(mLocalTieBreaker);

    // Add ICE-CONTROLLING attribute if needed
    if (mEnableControlling)
      msg.iceControllingAttr().setTieBreaker(mLocalTieBreaker);

    // Add ICE-PRIORITY attribute if needed
    if (mEnablePriority)
      msg.icePriorityAttr().setPriority(mPriority);
  }

  // Build packet
  msg.buildPacket(mOutgoingData, mPassword);
  
  // Copy comment
  msg.setComment(mComment);

  mComposed = true;
  
  return new ByteBuffer(mOutgoingData);
}

bool ServerBinding::hasUseCandidate()
{
  return mUseCandidate;
}

bool ServerBinding::gotRequest()
{
  return !mGenerate400 && !mGenerate487;
}

int ServerBinding::role()
{
  return mRole;
}

void ServerBinding::generate487()
{
  mGenerate487 = true;
}

std::string ServerBinding::remoteTieBreaker()
{
  return mRemoteTieBreaker;
}

void ServerBinding::restart()
{
}

//-------------------- BindingIndication ---------------
BindingIndication::BindingIndication(unsigned int interval)
:mInterval(interval)
{
  setComment("BindingIndication");
}

BindingIndication::~BindingIndication()
{
}

bool BindingIndication::processData(StunMessage& msg, NetworkAddress& address)
{
  return (msg.messageClass() == StunMessage::IndicationClass &&
          msg.messageType() == StunMessage::Binding);
}


ByteBuffer* BindingIndication::generateData(bool force)
{
  if (!mComposed)
  {
    StunMessage msg;
    msg.setMessageClass(StunMessage::IndicationClass);
    msg.setMessageType(StunMessage::Binding);
    msg.setTransactionId(mTransactionID);
    msg.setComment(mComment);
    enqueueMessage(msg);
    mComposed = true;
  }

  return Transaction::generateData(force);
}

