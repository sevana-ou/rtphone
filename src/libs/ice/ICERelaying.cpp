/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICERelaying.h"
#include "ICEStunAttributes.h"
#include "ICETime.h"
#include "ICEMD5.h"
#include "ICELog.h"
#include "ICEStream.h"

#include <assert.h>

using namespace ice;

#define LOG_SUBSYSTEM "ICE"
// ------------------ ClientAllocate -----------------

ClientAllocate::ClientAllocate(unsigned int lifetime)
:AuthTransaction(), mLifetime(lifetime), mWireFamily(AF_INET), mAllocFamily(AF_INET)
{
  assert(lifetime > 60 || !lifetime);
  setComment("ClientAllocate");
  addLongTermCredentials(true);
}

ClientAllocate::~ClientAllocate()
{
}

NetworkAddress& ClientAllocate::relayedAddress()
{
  return mRelayedAddr;
}

NetworkAddress& ClientAllocate::reflexiveAddress()
{
  return mReflexiveAddr;
}

NetworkAddress& ClientAllocate::responseAddress()
{
  return mResponseAddr;
}

void ClientAllocate::setInitialRequest(StunMessage& msg)
{
  if (keepalive())
  {
    // Build refresh request
    msg.setMessageType(ice::StunMessage::Refresh);
    msg.setMessageClass(StunMessage::RequestClass);
  }
  else
  {
    // Build allocate request
    msg.setMessageClass(StunMessage::RequestClass);
    msg.setMessageType(StunMessage::Allocate);
    
    // Add UDP transport attribute (RequestedTransport is UDP by default)
    msg.setAttribute(new RequestedTransport());

    // Add request family attribute
    if (mAllocFamily != mWireFamily)
      msg.setAttribute(new RequestedAddressFamily(mAllocFamily == AF_INET ? IPv4 : IPv6));
  }
}

void ClientAllocate::setAuthenticatedRequest(StunMessage& msg)
{
  if (keepalive())
  {
    // Build refresh request
    msg.setMessageType(ice::StunMessage::Refresh);
    msg.setMessageClass(StunMessage::RequestClass);
    msg.lifetimeAttr().setLifetime(mLifetime);
  }
  else
  {
    msg.setMessageClass(StunMessage::RequestClass);
    msg.setMessageType(StunMessage::Allocate);

    // Add UDP transport attribute
    msg.setAttribute(new RequestedTransport());

    // Add LIFETIME
    msg.lifetimeAttr().setLifetime(mLifetime);

    // Add request family attribute
    if (mAllocFamily != mWireFamily)
      msg.setAttribute(new RequestedAddressFamily(mAllocFamily == AF_INET ? IPv4 : IPv6));
  }
}

void ClientAllocate::processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress)
{
  if (msg.hasAttribute(StunAttribute::XorMappedAddress))
    mReflexiveAddr  = msg.xorMappedAddressAttr().address();

  if (msg.hasAttribute(StunAttribute::XorRelayedAddress))
    mRelayedAddr  = msg.xorRelayedAddressAttr().address();

  if (msg.hasAttribute(StunAttribute::Lifetime))
    mLifetime = msg.lifetimeAttr().lifetime();

  mResponseAddr = sourceAddress;
  ICELogDebug(<< "Allocated for " << (int)mLifetime << " seconds.");
}

int ClientAllocate::lifetime()
{
  return mLifetime;
}

void ClientAllocate::setWireFamily(int family)
{
  mWireFamily = family;
}

int ClientAllocate::getWireFamily() const
{
  return mWireFamily;
}

void ClientAllocate::setAllocFamily(int family)
{
  mAllocFamily = family;
}

int ClientAllocate::getAllocFamily() const
{
  return mAllocFamily;
}


//------------------- ClientRefresh -------------------------------
ClientRefresh::ClientRefresh(unsigned int lifetime, Stream* stream, ClientAllocate* allocate)
:AuthTransaction(), mLifetime(lifetime), mStream(stream)
{
  assert(stream);
  setComment("ClientRefresh");
  addLongTermCredentials(true);
  
  // Copy data from Allocate transaction
  if (allocate)
  {
    setDestination( allocate->destination() );
    setPassword( stream->mConfig.mTurnPassword );
    setUsername( stream->mConfig.mTurnUsername );
    setComponent( allocate->component() );
#ifdef ICE_CACHE_REALM_NONCE
    setRealm( allocate->realm() );
    setNonce( allocate->nonce() );
#endif
    mRelayed = allocate->relayedAddress();
    mReflexive = allocate->reflexiveAddress();
  }
}

ClientRefresh::~ClientRefresh()
{
}
  
void ClientRefresh::setInitialRequest(StunMessage& msg)
{
  ICELogDebug(<< "Prepare to run ClientRefresh on TURN server with lifetime " << mLifetime);
  msg.setMessageType(ice::StunMessage::Refresh);
  msg.setMessageClass(ice::StunMessage::RequestClass);
}

void ClientRefresh::setAuthenticatedRequest(StunMessage& msg)
{
  msg.setMessageType(StunMessage::Refresh);
  msg.setMessageClass(StunMessage::RequestClass);
  msg.lifetimeAttr().setLifetime(mLifetime);
}

void ClientRefresh::processSuccessMessage(StunMessage& /*msg*/, NetworkAddress& /*sourceAddress*/)
{
  if (mLifetime)
  {
    ICELogDebug(<< "TURN allocation refreshed for " << (int)mLifetime << " seconds.");
  }
  else
  {
    if (mStream)
    {
      if (mStream->mTurnAllocated > 0)
        mStream->mTurnAllocated--;
    }
    ICELogDebug(<< "TURN allocation is deleted.");
  }
}

void ClientRefresh::processError()
{
  if (mStream)
  {
    if (!mLifetime)
    {
      if (mStream->mTurnAllocated > 0)
        mStream->mTurnAllocated--;
      ICELogCritical(<< "TURN allocation is not deleted due to error " << errorCode() << " " << errorResponse());
    }
    else
      ICELogDebug(<< "ClientRefresh failed due to error " << errorCode() << " " << errorResponse());
  }
}

NetworkAddress ClientRefresh::relayedAddress()
{
  return mRelayed;
}

NetworkAddress ClientRefresh::reflexiveAddress()
{
  return mReflexive;
}

//------------------- ClientChannelBind ----------------------------

static TurnPrefix GPrefix = 0;
static Mutex GPrefixGuard;
static TurnPrefix obtainNewPrefix()
{
  Lock l(GPrefixGuard);
  
  // Generate initial value if needed
  if (!GPrefix)
    GPrefix = (rand() % (0x7FFE - 0x4000)) + 0x4000;
  
  // Avoid logical overflow
  if (GPrefix == 0x7FFE)
    GPrefix = 0x4000;

  return ++GPrefix;
}

ClientChannelBind::ClientChannelBind(const NetworkAddress& address)
:AuthTransaction(), mPeerAddress(address)
{
  setComment( "ClientChannelBind" );
  // Compute prefix
  mChannelPrefix = obtainNewPrefix();
  ICELogInfo(<< "Channel prefix for TURN channel bind is " << mChannelPrefix);
  addLongTermCredentials(true);
}

ClientChannelBind::~ClientChannelBind()
{
}

void ClientChannelBind::setInitialRequest(StunMessage& msg)
{
  msg.setMessageClass(StunMessage::RequestClass);
  msg.setMessageType(StunMessage::ChannelBind);
  
  msg.channelNumberAttr().setChannelNumber(mChannelPrefix);
  msg.xorPeerAddressAttr().address() = mPeerAddress;
}

void ClientChannelBind::setAuthenticatedRequest(StunMessage& msg)
{
  setInitialRequest(msg);
}

unsigned short ClientChannelBind::channelPrefix()
{
  return mChannelPrefix;
}

bool ClientChannelBind::processData(StunMessage& msg, NetworkAddress& address)
{
  return AuthTransaction::processData(msg, address);
}

void ClientChannelBind::processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress)
{
  ICELogDebug(<< mPeerAddress.toStdString() << " is bound to " << mChannelPrefix);
  // Make this transaction keepalive
  setKeepalive( true );
  setType( KeepAlive );
  setInterval( ICE_PERMISSIONS_REFRESH_INTERVAL );
  setTimestamp( ICETimeHelper::timestamp() );
}

void ClientChannelBind::processError()
{
  ICELogCritical(<< "Failed to bind channel with error " << errorCode());
}

NetworkAddress ClientChannelBind::peerAddress()
{
  return mPeerAddress;
}
//----------------- ClientCreatePermission ------------------------

ClientCreatePermission::ClientCreatePermission()
:AuthTransaction()
{
  setComment("ClientCreatePermission");
  addLongTermCredentials(true);
}

ClientCreatePermission::~ClientCreatePermission()
{
  ;
}

void ClientCreatePermission::addIpAddress(const NetworkAddress& ip)
{
  // Skip loopback / empty / LAN / IPv6 addresses addresses
  if (!ip.isLoopback() && !ip.isEmpty() && !ip.isLAN() && ip.family() == AF_INET)
  {
    for (unsigned i=0; i<mIPAddressList.size(); i++)  
      if (mIPAddressList[i].ip() == ip.ip())
        return;

    ICELogInfo( << "Permission is to be installed for " << ip.toStdString());
    mIPAddressList.push_back(ip);
  }
}

void ClientCreatePermission::processSuccessMessage(StunMessage& /*msg*/, NetworkAddress& /*sourceAddress*/)
{
  ICELogDebug(<< "Set permissions ok.");
  setKeepalive( true );
  setType( KeepAlive );
  setInterval( ICE_PERMISSIONS_REFRESH_INTERVAL );
  setTimestamp( ICETimeHelper::timestamp() );
}

void ClientCreatePermission::setInitialRequest(StunMessage& msg)
{
  msg.setMessageClass(StunMessage::RequestClass);
  msg.setMessageType(StunMessage::CreatePermission);
  
  for (unsigned i=0; i<mIPAddressList.size(); i++)
  {
    XorPeerAddress* xpa = new XorPeerAddress();
    xpa->address() = mIPAddressList[i];
    msg.addAttribute(xpa);
  }
}

void 
ClientCreatePermission::setAuthenticatedRequest(StunMessage& msg)
{
  setInitialRequest(msg);
}

ClientCreatePermission::IpList& ClientCreatePermission::ipList()
{
  return mIPAddressList;
}

// ------------------ SendIndication ----------------------------------
SendIndication::SendIndication()
{
}

SendIndication::~SendIndication()
{
}
    
void SendIndication::setTarget(NetworkAddress& addr)
{
  mTarget = addr;
}

NetworkAddress& SendIndication::target()
{
  return mTarget;
}

void SendIndication::setPlainData(ByteBuffer& plain)
{
  mPlainData = plain;
}

ByteBuffer& SendIndication::plainData()
{
  return mPlainData;
}

ByteBuffer* SendIndication::buildPacket()
{
  StunMessage m;
  m.setMessageClass(StunMessage::IndicationClass);
  m.setMessageType(StunMessage::Send);

  XorPeerAddress* xpa = new XorPeerAddress();
  xpa->address() = mTarget;

  DataAttribute* da = new DataAttribute();
  da->setData(mPlainData);

  m.setAttribute(xpa);
  m.setAttribute(da);

  ByteBuffer* result = new ByteBuffer();
  m.buildPacket(*result, "");
  
  return result;
}

ByteBuffer* SendIndication::buildPacket(NetworkAddress& target, ByteBuffer& data, NetworkAddress& relay, int component)
{
  SendIndication si;
  si.setTarget( target );
  si.setPlainData( data );
  
  ByteBuffer* result = si.buildPacket();
  result->setComponent( component );
  result->setRemoteAddress( relay );

  return result;
}
