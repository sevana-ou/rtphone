/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef EP_ACCOUNT_H
#define EP_ACCOUNT_H

#include "../helper/HL_Pointer.h"
#include "../helper/HL_VariantMap.h"
#include "ice/ICEAddress.h"
#include "ice/ICETime.h"
#include "ice/ICEBox.h"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "resip/stack/NameAddr.hxx"

#include "EP_Observer.h"

class UserAgent;
class Session;

class Account: public resip::DnsResultSink
{
friend class UserAgent;
friend class NATDecorator;
public:
  Account(PVariantMap config, UserAgent& agent);
  ~Account();

  void start();
  void stop();
  void refresh();
  bool active();
  int id() const;

  enum class RegistrationState
  {
    None,
    Registering,
    Reregistering,
    Registered,
    Unregistering
  };
  RegistrationState registrationState();

  /* Publishes new presence information */
  void publishPresence(bool online, const std::string& content, int seconds = 600);

  /* Stops publishing of presence */
  void stopPublish();

  /* Starts observing on specified target / package */
  PClientObserver observe(const std::string& target, const std::string& package, void* tag);

  /* Queues message to peer with specified mime type. Returns ID of message. */
  int sendMsg(const std::string& peer, const void* ptr, unsigned length, const std::string& mime, void* tag);

  /* Returns name of account - <sip:user@domain> */
  std::string name();

  /* Updates account with configuration */
  void setup(VariantMap& config);

  /* Returns corresponding resiprocate profile */
  resip::SharedPtr<resip::UserProfile> getUserProfile() const { return mProfile; }

  typedef std::map<std::string, std::string> UserInfo;
  void setUserInfo(const UserInfo& info);
  UserInfo getUserInfo() const;

protected:
  PVariantMap mConfig;

  // Registration
  ResipSession* mRegistration;
  resip::ClientRegistrationHandle mRegistrationHandle;
  resip::ClientPublicationHandle mPublication;
  resip::TransportType mUsedTransport;

  RegistrationState mRegistrationState;

  ice::NetworkAddress mExternalAddress;
  resip::SharedPtr<resip::UserProfile> mProfile;
  UserAgent& mAgent;
  bool mPresenceOnline;
  std::string mPresenceContent;

  // Timer to refresh STUN server IP
  ice::ICEScheduleTimer mRefreshStunServerIpTimer;

  // Cached auth
  resip::Auth mCachedAuth;

  // Id of account
  int mId;

  // User info about current state
  UserInfo mUserInfo;

  // List of client subscriptions sent from this account
  typedef std::set<PClientObserver> ClientObserverSet;
  ClientObserverSet mClientObserverSet;


  void process();
  // Method queries new stun server ip from dns (if stun server is specified as dns name)
  void queryStunServerIp();

  bool isResponsibleFor(const resip::NameAddr& addr);
  enum class SecureScheme
  {
    SipsAndTls,
    SipsOnly,
    TlsOnly,
    Nothing
  };

  resip::NameAddr contact(SecureScheme ss = SecureScheme::SipsOnly);

  // This method prepares configuration, creates ice stack and sets ownership to session
  void prepareIceStack(Session* session, ice::AgentRole role);
  void onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
  void onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response);
  void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& response);

#pragma region DnsResultSink implementation
  void onDnsResult(const resip::DNSResult<resip::DnsHostRecord>&);
  void onDnsResult(const resip::DNSResult<resip::DnsAAAARecord>&);
  void onDnsResult(const resip::DNSResult<resip::DnsSrvRecord>&);
  void onDnsResult(const resip::DNSResult<resip::DnsNaptrRecord>&);
  void onDnsResult(const resip::DNSResult<resip::DnsCnameRecord>&);
#pragma endregion

  static int generateId();
  static resip::AtomicCounter IdGenerator;
};

typedef std::shared_ptr<Account> PAccount;

#endif // EP_ACCOUNT_H
