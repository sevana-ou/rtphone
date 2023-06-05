/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_Engine.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Exception.h"
#include <resip/stack/ExtensionHeader.hxx>
#include <resip/stack/Pidf.hxx>
#include <resip/stack/PlainContents.hxx>

#define LOG_SUBSYSTEM "Account"

#define CONFIG(X) mConfig->at(X)
#define CONFIG_EXISTS(X) mConfig->exists(X)

//#define MODIFY_VIA_BEHIND_NAT

// NAT decorator
class NATDecorator: public resip::MessageDecorator
{
protected:
  UserAgent&      mUserAgent;
  resip::SipMessage mMessage;

  resip::Data     mViaHost;
  unsigned short  mViaPort;

  resip::Data     mContactsHost;
  resip::Data     mContactsScheme;
  unsigned short  mContactsPort;

public:
  NATDecorator(UserAgent& endpoint);  
  virtual ~NATDecorator();

  virtual void    decorateMessage(resip::SipMessage &msg, const resip::Tuple &source, const resip::Tuple &destination, const resip::Data& sigcompId);
  virtual void    rollbackMessage(resip::SipMessage& msg);
  virtual MessageDecorator* clone() const;
};


NATDecorator::NATDecorator(UserAgent& ua)
  :mUserAgent(ua), mViaPort(0), mContactsPort(0)
{
}

NATDecorator::~NATDecorator()
{
}

void NATDecorator::decorateMessage(resip::SipMessage &msg, const resip::Tuple &source, const resip::Tuple &destination, const resip::Data& sigcompId)
{
  // Make a copy to allow rollback
  mMessage = msg;

  std::stringstream dump;
  mMessage.encode(dump);
  //ICELogDebug(<< "Decorating message: \n" << dump.str());

  // Check From: header and find the account
  resip::NameAddr from;
  if (msg.isRequest())
    from = msg.header(resip::h_From);
  else
    from = msg.header(resip::h_To);

  PAccount account = mUserAgent.getAccount(from);
  if (!account)
  {
    ICELogDebug(<< "Bad from header " << from.uri().getAor().c_str() << ". Will skip it");
    return;
  }

  if (!account->mConfig->at(CONFIG_EXTERNALIP).asBool())
    return;

  if (!account->mExternalAddress.isEmpty())
  {
#ifdef MODIFY_VIA_BEHIND_NAT
    if (msg.header(resip::h_Vias).size() > 0)
    {
      resip::Via& via = msg.header(resip::h_Vias).front();
      mViaHost = via.sentHost();
      mViaPort = via.sentPort();

      via.sentHost() = resip::Data(account->mExternalAddress.ip());
      via.sentPort() = account->mExternalAddress.port();
    }
#endif

    if (msg.header(resip::h_Contacts).size() > 0)
    {
      resip::Uri& uri = msg.header(resip::h_Contacts).front().uri();
      mContactsHost = uri.host();
      mContactsPort = uri.port();
      mContactsScheme = uri.scheme();

      uri.host() = resip::Data(account->mExternalAddress.ip());
      uri.port() = account->mExternalAddress.port();
      if (account->mConfig->at(CONFIG_SIPS).asBool())
      {
        //uri.scheme() = "sips";
        //uri.param(resip::p_transport) = "tls";
      }

      //uri.scheme() = account->mConfig->at(CONFIG_SIPS).asBool() ? "sips" : "sip";
    }
  }
}

void NATDecorator::rollbackMessage(resip::SipMessage& msg)
{
  // Check From: header and find the account
  resip::NameAddr from = msg.header(resip::h_From);
  PAccount account = mUserAgent.getAccount(from);
  if (!account)
    return;

  if (!account->mExternalAddress.isEmpty())
  {
#ifdef MODIFY_VIA_BEHIND_NAT
    if (msg.header(resip::h_Vias).size() > 0)
    {
      resip::Via& via = msg.header(resip::h_Vias).front();
      if ((via.sentHost() == resip::Data(account->mExternalAddress.ip())) &&
        (via.sentPort() == account->mExternalAddress.port()))
      {
        via.sentHost() = mViaHost;
        via.sentPort() = mViaPort;
      }
    }
#endif

    if (msg.header(resip::h_Contacts).size() > 0)
    {
      resip::Uri& uri = msg.header(resip::h_Contacts).front().uri();
      if ((uri.host() == resip::Data(account->mExternalAddress.ip())) &&
        (uri.port() == account->mExternalAddress.port()))
      {
        uri.host() = mContactsHost;
        uri.port() = mContactsPort;
        //uri.scheme() = mContactsScheme;
      }
    }
  }
}

resip::MessageDecorator* NATDecorator::clone() const
{
  return new NATDecorator(mUserAgent);
}

Account::Account(PVariantMap config, UserAgent& agent)
  :mAgent(agent), mId(0), mConfig(config), mRegistrationState(RegistrationState::None),
    mRegistration(NULL)
{
  mProfile = std::make_shared<resip::UserProfile>(agent.mProfile);
  mId = Account::generateId();
  setup(*config);
}

Account::~Account()
{
}

void Account::setup(VariantMap &config)
{
  // Credentials
  
  if (!config.exists(CONFIG_USERNAME) || !config.exists(CONFIG_PASSWORD) || !config.exists(CONFIG_DOMAIN))
    throw Exception(ERR_NO_CREDENTIALS);

  mProfile->clearDigestCredentials();
  mProfile->setDigestCredential(resip::Data(config[CONFIG_DOMAIN].asStdString()),
    resip::Data(config[CONFIG_USERNAME].asStdString()),
    resip::Data(config[CONFIG_PASSWORD].asStdString()));
  ICELogInfo( << "Credentials are set to domain " << config[CONFIG_DOMAIN].asStdString() <<
    ", username to " << config[CONFIG_USERNAME].asStdString());

  // Proxy
  mProfile->unsetOutboundProxy();
  if (config.exists(CONFIG_PROXY))
  {
    if (!config[CONFIG_PROXY].asStdString().empty())
    {
      resip::Uri proxyAddr;
      proxyAddr.host() = resip::Data(config[CONFIG_PROXY].asStdString());
      proxyAddr.port() = 5060;
      if (config.exists(CONFIG_PROXYPORT))
      {
        if (config[CONFIG_PROXYPORT].asInt())
          proxyAddr.port() = config[CONFIG_PROXYPORT].asInt();
      }
      if (config[CONFIG_SIPS].asBool())
        proxyAddr.param(resip::p_transport) = "tls";

      mProfile->setOutboundProxy(proxyAddr);
    }
  }

  // NAT decorator
  mProfile->setOutboundDecorator(std::make_shared<NATDecorator>(mAgent));

  // Rinstance
  if (config.exists(CONFIG_INSTANCE_ID))
  {
    if (!config[CONFIG_INSTANCE_ID].asStdString().empty())
        mProfile->setInstanceId(config[CONFIG_INSTANCE_ID].asStdString().c_str());
  }
  else
    mProfile->setInstanceId(resip::Data::Empty);

  if (config.exists(CONFIG_REGID))
    mProfile->setRegId(config[CONFIG_REGID].asInt());

  if (config.exists(CONFIG_RINSTANCE))
    mProfile->setRinstanceEnabled(config[CONFIG_RINSTANCE].asBool());
  else
    mProfile->setRinstanceEnabled(true);

  if (config.exists(CONFIG_RPORT))
    mProfile->setRportEnabled(config[CONFIG_RPORT].asBool());
  else
    mProfile->setRportEnabled(true);

  // From header
  resip::NameAddr from;
  if (config.exists(CONFIG_DISPLAYNAME))
    from.displayName() = resip::Data(config[CONFIG_DISPLAYNAME].asStdString().c_str());

  from.uri().scheme() = config[CONFIG_SIPS].asBool() ? "sips" : "sip";
  if (config[CONFIG_DOMAINPORT].asInt() != 0)
    from.uri().port() = config[CONFIG_DOMAINPORT].asInt();
  else
    from.uri().port();// = 5060;

  from.uri().user() = resip::Data(config[CONFIG_USERNAME].asStdString());
  from.uri().host() = resip::Data(config[CONFIG_DOMAIN].asStdString());

  mProfile->setDefaultFrom(from);

  if (!CONFIG_EXISTS(CONFIG_REGISTERDURATION))
    CONFIG(CONFIG_REGISTERDURATION) = UA_REGISTRATION_TIME;
}

int Account::id() const
{
  return mId;
}

void Account::start()
{
  ICELogInfo(<< "Starting account " << this->name());
  if (mRegistrationState != RegistrationState::None)
  {
    ICELogInfo(<< "Registration is active or in progress already.");
    return;
  }
  if (!mAgent.mDum)
  {
    ICELogInfo(<< "DUM is not started yet.");
    return;
  }

  // Create registration
  mRegistration = new ResipSession(*mAgent.mDum);
  auto regmessage = mAgent.mDum->makeRegistration(mProfile->getDefaultFrom(), mProfile, mConfig->at(CONFIG_REGISTERDURATION).asInt(), mRegistration);

  for (UserInfo::const_iterator iter = mUserInfo.begin(); iter != mUserInfo.end(); iter++)
    regmessage->header(resip::ExtensionHeader(iter->first.c_str())).push_back(resip::StringCategory(iter->second.c_str()));

  mRegistrationState = RegistrationState::Registering;

  // Send packet here
  mAgent.mDum->send(regmessage);

  // Check if STUN IP is required
  bool noStunServerIp = !CONFIG_EXISTS(CONFIG_STUNSERVER_IP);
  //bool hasStunServerName = !CONFIG(CONFIG_STUNSERVER_NAME).asStdString().empty();
  if (noStunServerIp)
  {
    ICELogInfo(<<"No STUN server name or IP is not specified. Has to resolve/discover STUN server IP.");
    mRefreshStunServerIpTimer.start(CONFIG(CONFIG_DNS_CACHE_TIME).asInt() * 1000);
    mRefreshStunServerIpTimer.isTimeToSend();
    queryStunServerIp();
  }
}

void Account::stop()
{
  // Close presence publication
  if (mPublication.isValid())
  {
    mPublication->end();
    mPublication = resip::ClientPublicationHandle();
  }

  // Close client subscriptions

  // Close registration
  if (mRegistrationHandle.isValid())
  {
    mRegistrationHandle->removeAll();
    mRegistrationHandle = resip::ClientRegistrationHandle();
  }
  else
  if (mRegistration)
  {
    mRegistration->end();
  }
  mRegistration = NULL;
  mRegistrationState = RegistrationState::None;
}

void Account::refresh()
{
  if (mRegistrationHandle.isValid())
  {
    mRegistrationState = RegistrationState::Registering;
    mRegistrationHandle->requestRefresh();
  }

  if (mPublication.isValid())
  {
    mPublication->refresh();
  }
}

bool Account::active()
{
  return mRegistrationState == RegistrationState::Registered ||
      mRegistrationState == RegistrationState::Registering ||
      mRegistrationState == RegistrationState::Reregistering;
}

std::string Account::name()
{
  return contact(SecureScheme::Nothing).uri().toString().c_str();
}

Account::RegistrationState Account::registrationState()
{
  return mRegistrationState;
}

void Account::publishPresence(bool online, const std::string& content, int seconds)
{
  if (online == mPresenceOnline && content == mPresenceContent)
    return;

  mPresenceOnline = online;
  mPresenceContent = content;

  resip::Pidf p;
  p.setEntity(contact(SecureScheme::Nothing).uri());
  p.setSimpleId(resip::Data(CONFIG(CONFIG_PRESENCE_ID).asStdString()));
  p.setSimpleStatus(online, resip::Data(content));

  if (mPublication.isValid())
    mPublication->update(&p);
  else
    mAgent.mDum->send(mAgent.mDum->makePublication(contact(SecureScheme::TlsOnly), mProfile, p, resip::Symbols::Presence, seconds));
}

void Account::stopPublish()
{
  if (mPublication.isValid())
    mPublication->end();
}

PClientObserver Account::observe(const std::string& target, const std::string& package, void* tag)
{
  // Add subscription functionality
  PClientObserver observer(new ClientObserver());
  observer->mSession = new ResipSession(*mAgent.mDum);
  observer->mSession->setRemoteAddress(target);
  observer->mSession->setTag(tag);
  observer->mSessionId = observer->mSession->sessionId();
  observer->mPeer = target;

  std::shared_ptr<resip::SipMessage> msg;
  int expires = DEFAULT_SUBSCRIPTION_TIME, refresh = DEFAULT_SUBSCRIPTION_REFRESHTIME;
  if (mConfig->exists(CONFIG_SUBSCRIPTION_TIME))
    expires = CONFIG(CONFIG_SUBSCRIPTION_TIME).asInt();
  if (mConfig->exists(CONFIG_SUBSCRIPTION_REFRESHTIME))
    refresh = CONFIG(CONFIG_SUBSCRIPTION_REFRESHTIME).asInt();

  msg = mAgent.mDum->makeSubscription(resip::NameAddr(resip::Data(target)), mProfile,
    resip::Data(package), expires, refresh, observer->mSession);
  msg->header(resip::h_Accepts) = mAgent.mDum->getMasterProfile()->getSupportedMimeTypes(resip::NOTIFY);
  
  mAgent.mClientObserverMap[observer->mSessionId] = observer;
  mAgent.mDum->send(msg);

  return observer;
}

/* Queues message to peer with specified mime type. Returns ID of message. */
int Account::sendMsg(const std::string& peer, const void* ptr, unsigned length, const std::string& mime, void* tag)
{
  ResipSession* s = new ResipSession(*mAgent.mDum);
  s->setUa(&mAgent);
  s->setTag(tag);
  s->setRemoteAddress(peer);

  // Find MIME type
  resip::Mime type;
  std::string::size_type p = mime.find('/');
  if (p != std::string::npos)
    type = resip::Mime(resip::Data(mime.substr(0, p)), resip::Data(mime.substr(p+1)));
  else
    type = resip::Mime(resip::Data(mime), resip::Data());

  resip::ClientPagerMessageHandle msgHandle = mAgent.mDum->makePagerMessage(resip::NameAddr(resip::Data(peer)), mProfile, s);
  unique_ptr<resip::Contents> contentPtr(new resip::PlainContents(resip::Data(std::string((const char*)ptr, length)),type));
  int result = s->sessionId();
  msgHandle->page(std::move(contentPtr));

  return result;
}

resip::NameAddr Account::contact(SecureScheme ss)
{
  resip::NameAddr result;
  switch (ss)
  {
  case SecureScheme::Nothing:
    break;
  case SecureScheme::SipsOnly:
    result.uri().scheme() = mConfig->at(CONFIG_SIPS).asBool() ? "sips" : "sip";
    break;
  case SecureScheme::SipsAndTls:
    result.uri().scheme() = mConfig->at(CONFIG_SIPS).asBool() ? "sips" : "sip";
    if (mConfig->at(CONFIG_SIPS).asBool())
      result.uri().param(resip::p_transport) = "tls";
    break;
  case SecureScheme::TlsOnly:
    if (mConfig->at(CONFIG_SIPS).asBool())
      result.uri().param(resip::p_transport) = "tls";
    break;
  }

  result.uri().user() = resip::Data(mConfig->at(CONFIG_USERNAME).asStdString());
  result.uri().host() = resip::Data(mConfig->at(CONFIG_DOMAIN).asStdString());
  result.uri().port() = mConfig->at(CONFIG_DOMAINPORT).asInt();

  return result;
}

void Account::queryStunServerIp()
{
  ICELogInfo(<<"Looking for STUN/TURN server IP");

  if (!mConfig->exists(CONFIG_STUNSERVER_NAME))
  {
    // Send request to find STUN or TURN service
    std::string target = std::string(mConfig->at(CONFIG_RELAY).asBool() ? "_turn" : "_stun") + "._udp." + mConfig->at(CONFIG_DOMAIN).asStdString();

    // Start lookup
    mAgent.mStack->getDnsStub().lookup<resip::RR_SRV>(resip::Data(target), this);
  }
  else
  {
    // Check if host name is ip already
    std::string server = mConfig->at(CONFIG_STUNSERVER_NAME).asStdString();
    if (ice::NetworkAddress::isIp(server))
    {
      mConfig->at(CONFIG_STUNSERVER_IP) = server;
      mRefreshStunServerIpTimer.stop();
    }
    else
      mAgent.mStack->getDnsStub().lookup<resip::RR_A>(resip::Data(server), this);
  }
}

void Account::prepareIceStack(Session *session, ice::AgentRole icerole)
{
  ice::ServerConfig config;
  ice::NetworkAddress addr;
  addr.setIp(mConfig->at(CONFIG_STUNSERVER_IP).asStdString());
  if (mConfig->at(CONFIG_STUNSERVER_PORT).asInt())
    addr.setPort(mConfig->at(CONFIG_STUNSERVER_PORT).asInt());
  else
    addr.setPort(3478);

  config.mServerList4.push_back(addr);
  config.mRelay = mConfig->at(CONFIG_RELAY).asBool();
  if (mConfig->exists(CONFIG_ICETIMEOUT))
    config.mTimeout = mConfig->at(CONFIG_ICETIMEOUT).asInt();

  config.mUsername = mConfig->at(CONFIG_ICEUSERNAME).asStdString();
  config.mPassword = mConfig->at(CONFIG_ICEPASSWORD).asStdString();

  config.mUseIPv4 = mAgent.config()[CONFIG_IPV4].asBool();
  config.mUseIPv6 = mAgent.config()[CONFIG_IPV6].asBool();
  //config.mDetectNetworkChange = true;
  //config.mNetworkCheckInterval = 5000;

  session->mIceStack = std::shared_ptr<ice::Stack>(ice::Stack::makeICEBox(config));
  session->mIceStack->setEventHandler(session, this);
  session->mIceStack->setRole(icerole);
}

void Account::process()
{
  if (mRefreshStunServerIpTimer.isTimeToSend())
      queryStunServerIp();
}

void Account::onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage &response)
{
  // Save registration handle
  mRegistrationHandle = h;

  // Copy user info to registration handle
  for (UserInfo::iterator iter = mUserInfo.begin(); iter != mUserInfo.end(); iter++)
    mRegistrationHandle->setCustomHeader(resip::Data(iter->first.c_str()), resip::Data(iter->second.c_str()));

  // Get the Via
  const resip::Via& via = response.header(resip::h_Vias).front();

  // Get the sent host
  const resip::Data& sentHost = via.sentHost();//response.header(h_Contacts).front().uri().host();

  // Get the sentPort
  int sentPort = via.sentPort();

  const resip::Data& sourceHost = response.getSource().toData(resip::UDP);
  int rport = 0;
  if (via.exists(resip::p_rport))
    rport = via.param(resip::p_rport).port();

  resip::Data received = "";
  if (via.exists(resip::p_received))
    received = via.param(resip::p_received);

  bool hostChanged = sentHost != received && received.size() > 0;
  bool portChanged = sentPort != rport && rport != 0;

  // Save external port and IP address
  if (received.size() > 0 /*&& mConfig->at(CONFIG_EXTERNALIP).asBool()*/)
  {
    mExternalAddress.setIp(received.c_str());
    mExternalAddress.setPort(rport ? rport : sentPort);

    // Add new external address to domain list
    if (mAgent.mStack)
      mAgent.mStack->addAlias(resip::Data(mExternalAddress.ip()), mExternalAddress.port());
    if (mAgent.mDum)
      mAgent.mDum->addDomain(resip::Data(mExternalAddress.ip()));
  }

  mUsedTransport = response.getReceivedTransportTuple().getType();
  //bool streamTransport = mUsedTransport == resip::TCP || mUsedTransport == resip::TLS;

  // Retry registration for stream based transport too
  if  ( (hostChanged || portChanged) && mRegistrationState == RegistrationState::Registering /*&& !streamTransport*/ && mConfig->at(CONFIG_EXTERNALIP).asBool())
  {
    //mRegistrationHandle->requestRefresh();
    // Unregister at first
    mRegistrationHandle->removeAll();
    mRegistrationState = RegistrationState::Reregistering;
    return;
  }

  // So here we registered ok
  mRegistrationState = RegistrationState::Registered;
  mAgent.onAccountStart(mAgent.getAccount(this));
}

void Account::onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage &response)
{
  // Check if this unregistering is a part of rport pr
  if (mRegistrationState == RegistrationState::Reregistering)
  {
    //if (/*this->mUseExternalIP && */response.getSource().getType() == resip::UDP)
    {
      resip::Uri hostport(contact(SecureScheme::TlsOnly).uri());
      hostport.host() = resip::Data(mExternalAddress.ip());
      hostport.port() = mExternalAddress.port();
      if (mUsedTransport != resip::UDP)
      {
        const char* transportName = nullptr;
        switch (mUsedTransport)
        {
        case resip::TCP: transportName = "tcp"; break;
        case resip::TLS: transportName = "tls"; break;
        }

        hostport.param(resip::p_transport) = resip::Data(transportName);
      }
      mProfile->setOverrideHostAndPort(hostport);
      //mProfile->setDefaultFrom(from);
    }
    mProfile->setRegId(mConfig->at(CONFIG_REGID).asInt());
    auto regmessage = mAgent.mDum->makeRegistration(mProfile->getDefaultFrom(), mProfile, UA_REGISTRATION_TIME);
    for (UserInfo::const_iterator iter = mUserInfo.begin(); iter != mUserInfo.end(); iter++)
      regmessage->header(resip::ExtensionHeader(iter->first.c_str())).push_back(resip::StringCategory(iter->second.c_str()));

    mAgent.mDum->send(regmessage);
    return;
  }
  else
  {
    mRegistration = NULL;
    mRegistrationState = RegistrationState::None;
    mRegistrationHandle = resip::ClientRegistrationHandle();
    mAgent.onAccountStop(mAgent.getAccount(this), response.header(resip::h_StatusLine).statusCode());
  }
}

void Account::onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
  // Reset registration handle
  mRegistrationHandle = resip::ClientRegistrationHandle();
  mRegistrationState = RegistrationState::None;
  mRegistration = NULL;
  mAgent.onAccountStop(mAgent.getAccount(this), response.header(resip::h_StatusLine).statusCode());
}

void Account::onDnsResult(const resip::DNSResult<resip::DnsHostRecord>& result)
{
  if (result.status == 0)
  {
    resip::Data foundAddress = result.records.front().host();
    ICELogInfo( << "Success to resolve STUN/TURN address to " << foundAddress.c_str());
    mConfig->at(CONFIG_STUNSERVER_IP) = std::string(foundAddress.c_str());

    // Here the IP address of STUN/TURN server is found. If account is registered already - it means account is ready.
    if (mRegistrationState == RegistrationState::Registered)
      mAgent.onAccountStart(mAgent.getAccount(this));
  }
  else
  {
    ICELogError( << "Failed to resolve STUN or TURN server IP address.");
    if (mRegistrationState == RegistrationState::Registered)
    {
      int startCode = mConfig->at(CONFIG_STUNSERVER_NAME).asStdString().empty() ? 0 : 503;
      mAgent.onAccountStop(mAgent.getAccount(this), startCode);
    }
  }
}

void Account::onDnsResult(const resip::DNSResult<resip::DnsAAAARecord>&)
{

}

void Account::onDnsResult(const resip::DNSResult<resip::DnsSrvRecord>& result)
{
  if (result.status == 0)
  {
    // Find lowest priority
    int priority = 0x7FFFFFFF;
    for (size_t i=0; i<result.records.size(); i++)
      if (result.records[i].priority() < priority)
        priority = result.records[i].priority();

    size_t index = 0;
    int weight = 0;

    for (size_t i=0; i<result.records.size(); i++)
    {
      if (result.records[i].priority() == priority && result.records[i].weight() >= weight)
      {
        index = i;
        weight = result.records[i].weight();
      }
    }

    mConfig->at(CONFIG_STUNSERVER_PORT) = result.records[index].port();

    const char* host = result.records[index].target().c_str();

    ICELogInfo( << "Success to find STUN/TURN server on " << result.records[index].target().c_str() <<
      ":" << (int)result.records[index].port());


    if (inet_addr(host) == INADDR_NONE)
    {
      // Try to resolve domain name now
      mAgent.mStack->getDnsStub().lookup<resip::RR_A>(result.records[index].target(), this);
      //mStack->getDnsStub().lookup<resip::RR_AAAA>(result.records[index].target(), this);
    }
    else
    {
      mConfig->at(CONFIG_STUNSERVER_IP) = std::string(host);
    }
  }
  else
  {
    ICELogError( << "Failed to find STUN or TURN service for specified domain.");
    //mAgent::shutdown();
  }

}

void Account::onDnsResult(const resip::DNSResult<resip::DnsNaptrRecord>&)
{

}

void Account::onDnsResult(const resip::DNSResult<resip::DnsCnameRecord>&)
{

}

bool Account::isResponsibleFor(const resip::NameAddr &addr)
{
  std::string user = addr.uri().user().c_str();
  std::string domain = addr.uri().host().c_str();
  int p = addr.uri().port();
  if (mConfig->at(CONFIG_USERNAME).asStdString() == user && mConfig->at(CONFIG_DOMAIN).asStdString() == domain)
  {
    // Check if ports are the same or port is not specified at all
    if (mConfig->exists(CONFIG_DOMAINPORT))
      return mConfig->at(CONFIG_DOMAINPORT).asInt() == p || !p;
    else
      return true;
  }
  else
    return false;
}

void Account::setUserInfo(const UserInfo &info)
{
  mUserInfo = info;
  if (mRegistrationHandle.isValid())
  {
    for (UserInfo::iterator iter = mUserInfo.begin(); iter != mUserInfo.end(); iter++)
      mRegistrationHandle->setCustomHeader(resip::Data(iter->first.c_str()), resip::Data(iter->second.c_str()));
  }
}

Account::UserInfo Account::getUserInfo() const
{
  return mUserInfo;
}

std::atomic_int Account::IdGenerator;
int Account::generateId()
{
  return ++IdGenerator;
}
