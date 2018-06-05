/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_Observer.h"
#include "EP_Session.h"

#include <resip/stack/Pidf.hxx>
#include <resip/dum/ClientSubscription.hxx>

ClientObserver::ClientObserver()
{
}

ClientObserver::~ClientObserver()
{
}

void ClientObserver::refresh()
{
  if (mHandle.isValid())
    mHandle->requestRefresh();
}

void ClientObserver::stop()
{
  if (mHandle.isValid())
    mHandle->end();
  else
  if (mSession)
  {
    mSession->runTerminatedEvent(ResipSession::Type_Subscription);
    if (mSession)
      mSession->end();
  }
  mSession = NULL;
}

std::string ClientObserver::peer()
{
  return mPeer;
}

ServerObserver::ServerObserver()
  :mState(State_Incoming)
{

}

ServerObserver::~ServerObserver()
{
  stop();
}

std::string ServerObserver::peer() const
{
  return mPeer;
}

std::string ServerObserver::package() const
{
  return mPackage;
}

void ServerObserver::update(std::string simpleId, bool online, std::string msg)
{
  if (mState != State_Active)
    return;

  resip::Pidf p;
  p.setEntity(mContact);
  p.setSimpleId(resip::Data(simpleId));
  p.setSimpleStatus(online, resip::Data(msg));

  if (mHandle.isValid())
    mHandle->send(mHandle->update(&p));
}

void ServerObserver::accept()
{
  if (mHandle.isValid() && mState == State_Incoming)
  {
    mState = State_Active;
    mHandle->accept();
  }
}

void ServerObserver::stop()
{
  if (!mHandle.isValid())
    return;

  switch (mState)
  {
  case State_Incoming:
    mHandle->reject(404);
    break;
  case State_Active:
    mHandle->end();
    break;
  default:
    break;
  }
  mState = State_Closed;
}
