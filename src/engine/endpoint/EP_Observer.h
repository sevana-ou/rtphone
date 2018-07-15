/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EP_OBSERVER_H
#define EP_OBSERVER_H

#include "../helper/HL_Pointer.h"
#include "../helper/HL_VariantMap.h"
#include "ice/ICEAddress.h"
#include "ice/ICETime.h"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/stack/DnsInterface.hxx"

class UserAgent;
class Session;
class ResipSession;

class ClientObserver
{
  friend class Account;
  friend class UserAgent;
public:
  ClientObserver();
  ~ClientObserver();

  void refresh();
  void stop();
  std::string peer();

protected:
  resip::ClientSubscriptionHandle mHandle;
  ResipSession* mSession;
  int mSessionId;
  std::string mPeer;
};

typedef std::shared_ptr<ClientObserver> PClientObserver;

class ServerObserver
{
  friend class UserAgent;
public:
  ServerObserver();
  ~ServerObserver();

  std::string peer() const;
  std::string package() const;

  void accept();
  void update(std::string simpleId, bool online, std::string msg);
  void stop();

protected:
  enum State
  {
    State_Incoming,
    State_Active,
    State_Closed
  };
  State mState;
  resip::ServerSubscriptionHandle mHandle;
  std::string mPeer, mPackage;
  resip::Uri mContact;
  int mSessionId;
};

typedef std::shared_ptr<ServerObserver> PServerObserver;

#endif // EP_OBSERVER_H
