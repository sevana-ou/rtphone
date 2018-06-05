/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_RELAYING_H
#define __ICE_RELAYING_H

#include "ICEStunTransaction.h"
#include "ICEAddress.h"
#include "ICEAuthTransaction.h"
#include "ICEBinding.h"

namespace ice
{
  const int ChannelBindTimeout = 10 * 60 * 1000;
  const int PermissionTimeout = 5 * 60 * 1000;

  // This class encapsulates TURN Allocate transaction
  // Usage: create instance, set password and login, generate data.
  // pass incoming data while GetState() != StunTransaction::Failed or Success
    
  class ClientAllocate: public AuthTransaction
  {
  public:
    ClientAllocate(unsigned int lifetime);
    virtual ~ClientAllocate();
    
    NetworkAddress&     relayedAddress();
    NetworkAddress&     reflexiveAddress();
    NetworkAddress&     responseAddress();
    int             lifetime();
    void            setWireFamily(int family);
    int             getWireFamily() const;
    void            setAllocFamily(int family);
    int             getAllocFamily() const;
    
    virtual void    setInitialRequest(StunMessage& msg);
    virtual void    setAuthenticatedRequest(StunMessage& msg);
    virtual void    processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress);
    
  protected:
    NetworkAddress      mRelayedAddr;
    NetworkAddress      mReflexiveAddr;
    NetworkAddress      mResponseAddr;
    unsigned int        mLifetime;
    int                 mWireFamily;
    int                 mAllocFamily;
  };

  class ClientChannelBind: public AuthTransaction
  {
  public:
    ClientChannelBind(const NetworkAddress& peerAddress);
    virtual ~ClientChannelBind();

    /*Channel prefix must be 0x4000 through 0x7FFF: These values are the allowed channel
      numbers (16,383 possible values)
    */
    unsigned short  channelPrefix();
    
    void    setInitialRequest(StunMessage& msg);
    void    setAuthenticatedRequest(StunMessage& msg);
    void    processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress);
    bool    processData(StunMessage& msg, NetworkAddress& address);
    void    processError();
    NetworkAddress peerAddress();

  protected:
    unsigned short  mChannelPrefix;
    NetworkAddress      mPeerAddress;
  };

  class ClientCreatePermission: public AuthTransaction
  {
  public:
    typedef std::vector<NetworkAddress> IpList;

    ClientCreatePermission();
    virtual ~ClientCreatePermission();
    
    void            addIpAddress(const NetworkAddress& ip);

    virtual void    setInitialRequest(StunMessage& msg);
    virtual void    setAuthenticatedRequest(StunMessage& msg);
    virtual void    processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress);

    IpList& ipList();

  protected:
    IpList mIPAddressList;
  };
  
  struct Stream;
  class ClientRefresh: public AuthTransaction
  {
  public:
    ClientRefresh(unsigned int lifetime, Stream* stream, ClientAllocate* allocate = NULL);
    virtual ~ClientRefresh();
    
    virtual void    setInitialRequest(StunMessage& msg);
    virtual void    setAuthenticatedRequest(StunMessage& msg);
    virtual void    processSuccessMessage(StunMessage& msg, NetworkAddress& sourceAddress);
    virtual void    processError();
    unsigned int    lifetime() { return mLifetime; }
    NetworkAddress      relayedAddress();
    NetworkAddress      reflexiveAddress();
    
  protected:
    unsigned int    mLifetime;
    Stream*      mStream;
    NetworkAddress      mRelayed, mReflexive;
  };
  
  class SendIndication
  {
  public:
    SendIndication();
    ~SendIndication();
    
    void setTarget(NetworkAddress& addr);
    NetworkAddress& target();
    void setPlainData(ByteBuffer& plain);
    ByteBuffer& plainData();

    ByteBuffer* buildPacket();

    static ByteBuffer* buildPacket(NetworkAddress& target, ByteBuffer& data, NetworkAddress& relay, int component);

  protected:
    NetworkAddress mTarget;
    ByteBuffer mPlainData;
  };

}
#endif