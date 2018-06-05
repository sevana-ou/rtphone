/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUTH_TRANSACTION_H
#define __AUTH_TRANSACTION_H

#include "ICEPlatform.h"
#include "ICEStunTransaction.h"

namespace ice {

class AuthTransaction: public Transaction
{
public:
  AuthTransaction();
  virtual ~AuthTransaction();
  
  // Creates initial request, calls SetInitialRequest. This method is called once from GenerateData.
  void init();

  virtual void              setInitialRequest(StunMessage& msg) = 0;
  virtual void              setAuthenticatedRequest(StunMessage& msg) = 0;
  virtual void              processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress) = 0;
  virtual void              processError() {};
  
  virtual bool              processData(StunMessage& msg, NetworkAddress& address);
  virtual ByteBuffer*    generateData(bool force = false);

  int                       errorCode();
  std::string               errorResponse();
  virtual void              restart();
  virtual bool              hasToRunNow();

  std::string               realm() const;
  void                      setRealm(std::string realm);

  std::string               nonce() const;
  void                      setNonce(std::string nonce);

protected:
  bool                                mActive;
  bool                                mComposed;
  int                                 mErrorCode;
  std::string                         mErrorResponse;
  std::deque<SmartPtr<StunMessage> >  mOutgoingMsgQueue;
  unsigned char                       mKey[16];
  bool                                mConformsToKeepaliveSchedule;
  std::string                         mRealm;
  std::string                         mNonce;
  bool                                mCredentialsEncoded;

  void buildAuthenticatedMsg();
};

} // end of namespace

#endif