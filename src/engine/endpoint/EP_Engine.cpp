/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_Engine.h"
#include "EP_AudioProvider.h"
#include "../media/MT_AudioStream.h"

#include "../helper/HL_Exception.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_String.h"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TcpBaseTransport.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/MultipartRelatedContents.hxx"
#include "resip/stack/Rlmi.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/ssl/Security.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/XMLCursor.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/dum/InviteSession.hxx"

#if defined(TARGET_OSX)
# include "resip/stack/ssl/MacSecurity.hxx"
#endif

#if defined(TARGET_WIN)
# include "resip/stack/ssl/WinSecurity.hxx"
#endif

#define LOG_SUBSYSTEM "[Engine]"
#define LOCK Lock l(mGuard)
#define CAST2RESIPSESSION(x) (x.isValid() ? (x->getAppDialogSet().isValid() ? dynamic_cast<ResipSession*>(x->getAppDialogSet().get()) : NULL) : NULL)

typedef resip::SdpContents::Session::Medium Medium;
typedef resip::SdpContents::Session::MediumContainer MediumContainer;

//-------------- UserAgent -----------------------
UserAgent::UserAgent()
{
  mStack = nullptr;
  mDum = nullptr;

  mConfig[CONFIG_PRESENCE_ID] = std::string("device");
  mConfig[CONFIG_RTCP_ATTR] = true;

  // Create master profile
  mProfile = resip::SharedPtr<resip::MasterProfile>(new resip::MasterProfile());
  mProfile->clearSupportedMethods();
  mProfile->addSupportedMethod(resip::INVITE);
  mProfile->addSupportedMethod(resip::BYE);
  mProfile->addSupportedMethod(resip::CANCEL);
  mProfile->addSupportedMethod(resip::REGISTER);
  mProfile->addSupportedMethod(resip::NOTIFY);
  mProfile->addSupportedMethod(resip::SUBSCRIBE);
  mProfile->addSupportedMethod(resip::OPTIONS);
  mProfile->addSupportedMethod(resip::UPDATE);
  mProfile->addSupportedMethod(resip::MESSAGE);
  mProfile->addSupportedMethod(resip::ACK);
  mProfile->clearDigestCredentials();
  mProfile->unsetOutboundProxy();
}

UserAgent::~UserAgent()
{
  LOCK;

  shutdown();
  mAccountSet.clear();
  mClientObserverMap.clear();
  mServerObserverMap.clear();
  mSessionMap.clear();
}


void UserAgent::start()
{
  ICELogInfo(<< "Attempt to start endpoint.");
  LOCK;
  if (mStack)
  {
    ICELogError(<<"Endpoint is started already.");
    return;
  }
  
  // Initialize resip loggег
  resip::Log::initialize(resip::Log::OnlyExternal, resip::Log::Info, "Client", *this);

  // Build list of nameservers if specified
  resip::DnsStub::NameserverList nslist;

  if (mConfig.exists(CONFIG_OWN_DNS))
  {
    std::string line, t = mConfig[CONFIG_OWN_DNS].asStdString();
    std::stringstream ss(t);

    while (std::getline(ss, line))
    {
      line = StringHelper::trim(line);
      ice::NetworkAddress addr(line.c_str(), 0); addr.setPort(80); // Fake port to make ICEAddress initialized
      switch (addr.family())
      {
      case AF_INET:   nslist.push_back(resip::GenericIPAddress(*addr.sockaddr4())); break;
      case AF_INET6:  nslist.push_back(resip::GenericIPAddress(*addr.sockaddr6())); break;
      }
    }
  }
  
  // Security will be deleted in stop() method (in Stack destructor)
  resip::Security* s = nullptr;
#if defined(TARGET_WIN)
  s = new resip::WinSecurity();
#elif defined(TARGET_OSX)
  s = new resip::MacSecurity();
#elif defined(TARGET_LINUX)
  s = new resip::Security("/etc/ssl/certs");
#elif defined(TARGET_ANDROID)
  s = new resip::Security();
#endif
  mStack = new resip::SipStack(s, nslist);

  // Add root cert if specified
  if (mConfig.exists(CONFIG_ROOTCERT))
  {
    try
    {
      resip::Data cert = resip::Data(mConfig[CONFIG_ROOTCERT].asStdString());
      mStack->getSecurity()->addRootCertPEM(cert);
    }
    catch(resip::BaseException& /*e*/)
    {
      ICELogError(<< "Failed to preload root certificate");
    }
  }

  // Add transports
  mTransportList.clear();
  resip::InternalTransport* t;

#define ADD_TRANSPORT4(X) if ((t = dynamic_cast<resip::InternalTransport*>(mStack->addTransport(X, 0, resip::V4)))) { t->setTransportLogger(this); mTransportList.push_back(t);}
#define ADD_TRANSPORT6(X) if ((t = dynamic_cast<resip::InternalTransport*>(mStack->addTransport(X, 0, resip::V6)))) { t->setTransportLogger(this); mTransportList.push_back(t);}

  switch (mConfig[CONFIG_TRANSPORT].asInt())
  {
  case 0:
    if (mConfig[CONFIG_IPV4].asBool())
    {
      ADD_TRANSPORT4(resip::TCP)
      ADD_TRANSPORT4(resip::UDP)
      ADD_TRANSPORT4(resip::TLS)
    }
    if (mConfig[CONFIG_IPV6].asBool())
    {
      ADD_TRANSPORT6(resip::TCP)
      ADD_TRANSPORT6(resip::UDP)
      ADD_TRANSPORT6(resip::TLS)
    }
    break;

  case 1:
    if (mConfig[CONFIG_IPV4].asBool())
      ADD_TRANSPORT4(resip::UDP);
    if (mConfig[CONFIG_IPV6].asBool())
      ADD_TRANSPORT6(resip::UDP);
    break;

  case 2:
    if (mConfig[CONFIG_IPV4].asBool())
      ADD_TRANSPORT4(resip::TCP);
    if (mConfig[CONFIG_IPV6].asBool())
      ADD_TRANSPORT6(resip::TCP);
    break;

  case 3:
    if (mConfig[CONFIG_IPV4].asBool())
      ADD_TRANSPORT4(resip::TLS);
    if (mConfig[CONFIG_IPV6].asBool())
      ADD_TRANSPORT6(resip::TLS);
    break;

  default:
    assert(0);
  }
  
  mDum = new resip::DialogUsageManager(*mStack);

  // Set the name of user agent
  if (mConfig[CONFIG_USERAGENT].asBool())
    mProfile->setUserAgent(mConfig[CONFIG_USERAGENT].asStdString().c_str());
  
  // Configure rport
  mProfile->setRportEnabled(mConfig[CONFIG_RPORT].asBool());
  
  // Set keep-alive packets interval
  if (mConfig.exists(CONFIG_KEEPALIVETIME))
    mProfile->setKeepAliveTimeForDatagram(mConfig[CONFIG_KEEPALIVETIME].asInt());
  else
    mProfile->setKeepAliveTimeForDatagram(30);

  int dnsCacheTime = mConfig.exists(CONFIG_DNS_CACHE_TIME) ? mConfig[CONFIG_DNS_CACHE_TIME].asInt() : 86400;
  mStack->getDnsStub().setDnsCacheSize(65536);
  mStack->getDnsStub().setDnsCacheTTL(dnsCacheTime);

  // Disable statistics
  mStack->statisticsManagerEnabled() = false;

  // Allow wildcard certificates
  mStack->getSecurity()->setAllowWildcardCertificates(true);

  // Add special body type
  mProfile->addSupportedMimeType(resip::MESSAGE, resip::Mime("text", "plain"));
  mProfile->addSupportedMimeType(resip::INVITE, resip::Mime("application", "ddp"));
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Pidf::getStaticType());
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Mime("application", "simple-message-summary"));
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Mime("message", "sipfrag"));  
  mProfile->addSupportedMimeType(resip::OPTIONS, resip::Mime("application", "sdp")); 
  
  // xcap/rls
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Mime("application", "xcap-diff+xml"));
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Mime("application", "rlmi+xml"));
  mProfile->addSupportedMimeType(resip::NOTIFY, resip::Mime("multipart", "related"));
  mProfile->addSupportedOptionTag(resip::Token("eventlist"));
  
  mDum->setMasterProfile(mProfile);
  mDum->setClientRegistrationHandler(this);
  mDum->setClientAuthManager(unique_ptr<resip::ClientAuthManager>(new resip::ClientAuthManager()));

  mDum->addClientSubscriptionHandler(resip::Symbols::Presence, this);
  mDum->addClientSubscriptionHandler(resip::Data("message-summary"), this);
  mDum->addServerSubscriptionHandler(resip::Symbols::Presence, this);
  mDum->addServerSubscriptionHandler("refer", this);
  mDum->addClientSubscriptionHandler("refer", this);
  mDum->addClientPublicationHandler(resip::Symbols::Presence, this);
  mDum->setInviteSessionHandler(this);
  mDum->setClientPagerMessageHandler(this);
  mDum->setServerPagerMessageHandler(this);

  unique_ptr<resip::AppDialogSetFactory> uac_dsf(new ResipSessionFactory(this));
  mDum->setAppDialogSetFactory(uac_dsf);

  // Fire onStart event if stun is not used or stun server ip is known
  if (mConfig[CONFIG_STUNSERVER_NAME].asStdString().empty() || !mConfig[CONFIG_STUNSERVER_IP].asStdString().empty())
    onStart(0);
}


void UserAgent::shutdown()
{
  if (!mStack)
    return;

  ICELogInfo( << "Attempt to stop endpoint.");

  {
    LOCK;
    for (auto observerIter: mClientObserverMap)
      observerIter.second->stop();

    for (auto observerIter: mServerObserverMap)
      observerIter.second->stop();

    for (auto sessionIter: mSessionMap)
      sessionIter.second->stop();

    for (auto accountIter: mAccountSet)
      accountIter->stop();
  }
}

bool UserAgent::active()
{
  LOCK;
  return mStack != NULL;
}

void UserAgent::refresh()
{
  LOCK;

  for (auto acc: mAccountSet)
    acc->refresh();

  for (auto observer: mClientObserverMap)
    observer.second->refresh();
}

void UserAgent::onDumCanBeDeleted()
{
  delete mDum; mDum = NULL;
  delete mStack; mStack = NULL;
  
  mClientObserverMap.clear();
  mServerObserverMap.clear();

  // Generate onStop event here
  onStop();
}

void UserAgent::stop()
{
  LOCK;
  if (!mStack || !mDum)
    return;

  // Clear transport list to avoid races
  mTransportList.clear();

  // Dump statistics here
  ICELogInfo(<< "Remaining " << Session::InstanceCounter.value() << " session(s), " << ResipSession::InstanceCounter.value() << " resip DialogSet(s), " << resip::ClientRegistration::InstanceCounter.value() << " ClientRegistration(s)");

  mDum->shutdown(this);
  onDumCanBeDeleted();

}


void UserAgent::process()
{
  resip::FdSet fdset;
  if (mStack && mDum)
  {
    bool connectionFailed = false;

    mStack->buildFdSet(fdset);
    //unsigned int t1 = mStack->getTimeTillNextProcessMS();
    int ret = fdset.selectMilliSeconds(0);
    if (ret >= 0) // Got any results or time to send new packets?
    {
      Lock l(mGuard);
      //ICELogDebug(<< "Smth on SIP socket(s)");
      mStack->process(fdset);

      // Check if there failed connections
      for (unsigned transportIndex = 0; transportIndex < mTransportList.size(); transportIndex++)
        if (mTransportList[transportIndex]->getConnectionsDeleted())
        {
          connectionFailed = true;
          mTransportList[transportIndex]->resetConnectionsDeleted();
        }
    }
        
    {
      Lock l(mGuard);
      while (mDum->process())
        ;
    }

    if (connectionFailed)
      this->onSipConnectionFailed();
  }

  // Erase one terminated session. The rule is : seession must not have references from resiprocate and reference count has to be 1.
  {
    Lock l(mGuard);
    SessionMap::iterator sessionIter;
    for (sessionIter = mSessionMap.begin(); sessionIter != mSessionMap.end(); ++sessionIter)
    {
      if (sessionIter->second.use_count() == 1 && !sessionIter->second->mResipSession)
      {
        mSessionMap.erase(sessionIter);
        break;
      }
    }
  }

#pragma region ICE packet generating
  // Iterate available sessions
  {
    Lock l(mGuard);
      
    // Find all sessions
    std::set<int> idSet;
    SessionMap::iterator sessionIter;
    for (sessionIter = mSessionMap.begin(); sessionIter != mSessionMap.end(); ++sessionIter)
      idSet.insert(sessionIter->first);

    // Now process session one by one checking if current is available yet
    std::set<int>::iterator resipIter;
    for (resipIter = idSet.begin(); resipIter != idSet.end(); ++resipIter)
    {
      SessionMap::iterator sessionIter = mSessionMap.find(*resipIter);
      if (sessionIter == mSessionMap.end())
        continue;

      // Extract reference to session
      Session& session = *sessionIter->second;
        
      // Send queued offers if possible
      session.processQueuedOffer();

      // Generate outgoing data while available
      int iceStreamId = -1, iceComponentId = -1; void* iceTag = NULL; bool iceResponse;
      ice::PByteBuffer buffer;
      while ((buffer = session.mIceStack->generateOutgoingData(iceResponse, iceStreamId, iceComponentId, iceTag)))
      {
        // Find corresponding data provider
        for (unsigned i=0; i < session.mStreamList.size(); ++i)
        {
          Session::Stream& stream = session.mStreamList[i];

          if (stream.provider() && stream.iceInfo().mStreamId == iceStreamId)
          {
            // Send generated packet via provider's method to allow custom scheme of encryption
            ICELogDebug(<<"Sending ICE packet to " << buffer->remoteAddress().toStdString() << " with " << buffer->comment());

            PDatagramSocket s = iceComponentId == ICE_RTP_ID ? stream.socket4().mRtp : stream.socket4().mRtcp;
            stream.provider()->sendData(s, buffer->remoteAddress(), buffer->data(), buffer->size());
            break;
          }
        } // end of provider iterating
      }
    }
  }
#pragma endregion

  // TODO: see if there expired sessions - stopped and awaiting turn resources deallocation
}

void UserAgent::addRootCert(const ByteBuffer& data)
{
  LOCK;
  resip::Data b(data.data(), data.size());
  mStack->getSecurity()->addRootCertPEM(b);
}

PAccount UserAgent::createAccount(PVariantMap config)
{
  PAccount account(new Account(config, *this));
  mAccountSet.insert(account);
  return account;
}

void UserAgent::deleteAccount(PAccount account)
{
  // Delete reference from internal list
  AccountSet::iterator accountIter = mAccountSet.find(account);
  if (accountIter != mAccountSet.end())
    mAccountSet.erase(accountIter);
}

PSession UserAgent::createSession(PAccount account)
{
  LOCK;

  // Create session object. ICE stack will be created as member of DemoAppDialogSet
  PSession session(new Session(account));
  session->setUserAgent(this);
  
  // Save reference to session
  mSessionMap[session->id()] = session;

  // Create ICE stack and configure it
  account->prepareIceStack(session.get(), ice::RoleControlling);

  return session;
}

std::string UserAgent::formatSipAddress(std::string sip)
{
  std::string result;
  if (sip.size())
  {
    if (sip.find("sip:") == std::string::npos && sip.find("sips:") == std::string::npos)
      result = "<sip:" + sip + ">";
    else
    if (sip[0] != '<' && sip.find('<') == std::string::npos)
      result = "<" + sip + ">";
    else
      result = sip;
  }
  return result;
}

bool UserAgent::isSipAddressValid(std::string sip)
{
  bool result = false;
  try
  {
    if (sip.find('<') == std::string::npos)
      sip = "<" + sip;
    if (sip.find('>') == std::string::npos)
      sip += ">";

    resip::Data d(formatSipAddress(sip));
    resip::Uri uri(d);
    result = uri.isWellFormed();
    if (result)
    {
      if (uri.user().empty() || uri.host().empty())
        result = false;
    }
  }
  catch (...)
  {
    return result;
  }
  return result;
}

UserAgent::SipAddress UserAgent::parseSipAddress(const std::string& sip)
{
  SipAddress result;

  result.mValid = isSipAddressValid(sip);
  if (result.mValid)
  {
    resip::Data d(formatSipAddress(sip));
    resip::NameAddr nameaddr(d);
    //resip::Uri uri(d);
    if (!nameaddr.isWellFormed())
      result.mValid = false;
    else
    {
      result.mUsername = nameaddr.uri().user().c_str();
      result.mDomain = nameaddr.uri().host().c_str();
      if (nameaddr.uri().port())
      {
        char porttext[32];
        sprintf(porttext, ":%u", (unsigned)nameaddr.uri().port());
        result.mDomain += porttext;
      }

      result.mScheme = nameaddr.uri().scheme().c_str();
      if (result.mScheme.find('<') == 0)
        result.mScheme.erase(0, 1);
      result.mDisplayname = nameaddr.displayName().c_str();

      result.mValid &= !result.mUsername.empty() && !result.mDomain.empty();
    }
  }
  return result;
}

bool UserAgent::compareSipAddresses(std::string sip1, std::string sip2)
{
  if (sip1.empty() || sip2.empty())
    return false;

  resip::Data d1(formatSipAddress(sip1)), d2(formatSipAddress(sip2));
  resip::Uri uri1(d1), uri2(d2);
  
  return uri1.getAorNoPort().uppercase() == uri2.getAorNoPort().uppercase();
}


void UserAgent::onGathered(PSession s)
{
  ICELogInfo(<< "Session " << s->sessionId() << " gathered candidates");
}

// Called when new candidate is gathered
void UserAgent::onCandidateGathered(PSession s, const char* address)
{
  ICELogInfo(<< "Session " << s->sessionId() << " gathered new candidate " << address);
}


// Called when new connectivity check is finished
void UserAgent::onCheckFinished(PSession s, const char* description)
{

}

void UserAgent::onLog(const char* msg)
{
}

void UserAgent::sendOffer(Session* session)
{
  assert(session->mResipSession);

  // Construct SDP
  resip::SdpContents sdp;
  session->buildSdp(sdp, Sdp_Offer);
  
  if (session->mOriginVersion == 1)
  {
    // Construct INVITE session
    resip::SharedPtr<resip::SipMessage> msg = mDum->makeInviteSession(session->mRemotePeer, session->account()->mProfile, &sdp, session->mResipSession);

    // Include user headers
    for (Session::UserHeaders::const_iterator iter = session->mUserHeaders.begin(); iter != session->mUserHeaders.end(); iter++)
    {
      const std::string& name = iter->first;
      const std::string& value = iter->second;

      msg->header(resip::ExtensionHeader(name.c_str())).push_back(resip::StringCategory(value.c_str()));
    }
    mDum->send(msg);
  }
  else
  {
    // Send SDP
    resip::InviteSession* h = dynamic_cast<resip::InviteSession*>(session->mInviteHandle.get());
    if (h)
      h->provideOffer(sdp);
    else
      ICELogError(<< "No cast to InviteSession");
  }
}

#pragma region Registration handler

void UserAgent::onSuccess(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
  ICELogInfo (<< "Registration got 200 response.");
  Lock l(mGuard);

  // Find account by registration handle
  PAccount account = getAccount(response.header(resip::h_From));
  if (account)
    account->onSuccess(h, response);
}

// Called when all of my bindings have been removed
void UserAgent::onRemoved(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
  ICELogInfo( << "Registration is removed.");


  Lock l(mGuard);
  for (AccountSet::iterator accountIter = mAccountSet.begin(); accountIter != mAccountSet.end(); accountIter++)
    if ((*accountIter)->getUserProfile() == h->getUserProfile())
      (*accountIter)->onRemoved(h, response);
}
      
/// call on Retry-After failure. 
/// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
int UserAgent::onRequestRetry(resip::ClientRegistrationHandle h, int retrySeconds, const resip::SipMessage& response)
{
  return -1;
}

// Called if registration fails, usage will be destroyed (unless a 
// Registration retry interval is enabled in the Profile)
void UserAgent::onFailure(resip::ClientRegistrationHandle h, const resip::SipMessage& response)
{
  ICELogInfo (<< "Registration failed with code " << response.header(resip::h_StatusLine).statusCode());

  Lock l(mGuard);
  PAccount account = getAccount(response.header(resip::h_From));
  if (account)
    account->onFailure(h, response);
}


#pragma endregion

bool UserAgent::operator()(resip::Log::Level level, const resip::Subsystem& subsystem, const resip::Data& appName, const char* file, 
      int line, const resip::Data& message, const resip::Data& messageWithHeaders)
{
  std::string filename = file;
  std::stringstream ss;

  ss << "File "  << StringHelper::extractFilename(filename).c_str() << ", line " << line << ": " << message.c_str();
  if (level <= resip::Log::Crit)
      ICELogCritical(<< ss.str())
  else
  if (level <= resip::Log::Warning)
    ICELogError(<< ss.str().c_str())
  else
  if (level < resip::Log::Debug)
    ICELogInfo(<< ss.str().c_str())
  else
    ICELogDebug(<< ss.str().c_str())
  return false;
}

#pragma region INVITE handler


/// called when an initial INVITE or the intial response to an outoing invite  
void UserAgent::onNewSession(resip::ClientInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg)
{
}

void UserAgent::onNewSession(resip::ServerInviteSessionHandle h, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg)
{
  ResipSession* rs = CAST2RESIPSESSION(h);
  if (!rs)
  {
    h->reject(503);
    return;
  }

  // Find account
  PAccount account = getAccount(h->myAddr());
  if (!account)
  {
    h->reject(503);
    return;
  }

  // Bring new user session
  PSession s = createSession(account);

  rs->setSession(s.get());
  mSessionMap[s->sessionId()] = s;
  
  // Save remote address
  s->setRemoteAddress(h->peerAddr().uri().getAor().c_str());

  ICELogInfo( << "Session " << s->sessionId() << ": incoming.");

  h->provisional(100);

  // Create ICE stack and configure it
  if (account)
    account->prepareIceStack(s.get(), ice::RoleControlled);
}

/// Received a failure response from UAS
void UserAgent::onFailure(resip::ClientInviteSessionHandle, const resip::SipMessage& msg)
{
}

      
/// called when an in-dialog provisional response is received that contains an SDP body
void UserAgent::onEarlyMedia(resip::ClientInviteSessionHandle h, const resip::SipMessage&, const resip::SdpContents&)
{
}


/// called when dialog enters the Early state - typically after getting 18x
void UserAgent::onProvisional(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg)
{
  PSession s = getUserSession(CAST2RESIPSESSION(h)->mSessionId);
  if (!s)
    return;

  if (msg.isResponse())
  {
    int responseCode = msg.header(resip::h_StatusLine).statusCode();
    onSessionProvisional(s, responseCode);
  }

}


/// called when a dialog initiated as a UAC enters the connected state
void UserAgent::onConnected(resip::ClientInviteSessionHandle h, const resip::SipMessage& msg)
{
  PSession s = getUserSession(CAST2RESIPSESSION(h)->mSessionId);
  if (!s)
    return;

  if (!s->mOfferAnswerCounter)
  {
    ICELogInfo (<< "Session " << s->sessionId() << ": connected.");

    // Transfer user headers
    if (h.isValid())
      h->setUserHeaders(s->mUserHeaders);

    onSessionEstablished(s, EV_SIP, RtpPair<InternetAddress>());
    
    for (unsigned i=0; i<s->mStreamList.size(); i++)
    {
      if (s->mStreamList[i].provider())
        s->mStreamList[i].provider()->sessionEstablished(EV_SIP);
    }
  }
  s->mOfferAnswerCounter++;
}


/// called when a dialog initiated as a UAS enters the connected state
void UserAgent::onConnected(resip::InviteSessionHandle h, const resip::SipMessage& msg)
{
  ResipSession* resipSession = dynamic_cast<ResipSession*>(h->getAppDialogSet().get());
  if (!resipSession)
    return;
  PSession s = getUserSession(CAST2RESIPSESSION(h)->mSessionId);

  if (!s)
    return;

  ICELogInfo (<< "Session " << s->mSessionId << ": connected.");

  // Transfer user headers
  if (h.isValid())
    h->setUserHeaders(s->mUserHeaders);

  onSessionEstablished(s, EV_SIP, RtpPair<InternetAddress>());

  for (unsigned i=0; i<s->mStreamList.size(); i++)
  {
    if (s->mStreamList[i].provider())
      s->mStreamList[i].provider()->sessionEstablished(EV_SIP);
  }
}


void UserAgent::onTerminated(resip::InviteSessionHandle h, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* related)
{
  PSession s = getUserSession(CAST2RESIPSESSION(h)->mSessionId);
  if (!s)
    return;
  
  ICELogInfo( << "Session " << s->mSessionId << ": terminated.");

  int errorcode = 0;
  if (related)
  {
    if (related->isResponse())
      errorcode = related->header(resip::h_StatusLine).statusCode();
  }
  
  if (s->mResipSession)
    s->mResipSession->runTerminatedEvent(ResipSession::Type_Call, errorcode, (int)reason);
  s->clearProviders();

  // TODO: run turn resource deallocation sequence here. Otherwise release user session object.
}

/// called when a fork that was created through a 1xx never receives a 2xx
/// because another fork answered and this fork was canceled by a proxy. 
void UserAgent::onForkDestroyed(resip::ClientInviteSessionHandle)
{
}


/// called when a 3xx with valid targets is encountered in an early dialog     
/// This is different then getting a 3xx in onTerminated, as another
/// request will be attempted, so the DialogSet will not be destroyed.
/// Basically an onTermintated that conveys more information.
/// checking for 3xx respones in onTerminated will not work as there may
/// be no valid targets.
void UserAgent::onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg)
{
}

/// Called when an SDP answer is received - has nothing to do with user
/// answering the call 
void UserAgent::onAnswer(resip::InviteSessionHandle h, const resip::SipMessage& msg, const resip::SdpContents& sdp)
{
  // Check if session casts ok
  ResipSession* resipSession = dynamic_cast<ResipSession*>(h->getAppDialogSet().get());
  if (!resipSession)
    return;
  Session* s = resipSession->session();

  bool iceAvailable = true;

  ICELogInfo( << "Session " << s->mSessionId << ": got answer.");

  // Check for remote ICE credentials
  std::string icePwd, iceUfrag;
  if (sdp.session().exists("ice-pwd"))
    icePwd = sdp.session().getValues("ice-pwd").front().c_str();
  if (sdp.session().exists("ice-ufrag"))
    iceUfrag = sdp.session().getValues("ice-ufrag").front().c_str();

  if (s->mStreamList.size() < sdp.session().media().size())
  {
    ICELogError( << "SDP answer has wrong number of streams");
    h->end();
    return;
  }

  // Get default remote IP
  std::string remoteDefaultIP;
  if (sdp.session().isConnection())
    remoteDefaultIP = sdp.session().connection().getAddress().c_str();

  bool mediasupported = false;

  // Iterate SDP's streams
  std::list<resip::SdpContents::Session::Medium>::const_iterator mediaIter;
  unsigned streamIndex = 0;
  for (mediaIter = sdp.session().media().begin(), streamIndex = 0; 
    mediaIter != sdp.session().media().end() && streamIndex < s->mStreamList.size(); 
    ++mediaIter, ++streamIndex)
  {
    Session::Stream& stream = s->mStreamList[streamIndex];
    const resip::SdpContents::Session::Medium& remoteStream = *mediaIter;
    
    // Update remote default ip if available
    const std::list<resip::SdpContents::Session::Connection>& streamConnections = remoteStream.getMediumConnections();
    if (streamConnections.size())
      remoteDefaultIP = streamConnections.front().getAddress().c_str();
      
    if (remoteDefaultIP.empty())
      continue;

    // Check if stream is active
    if (remoteStream.exists("inactive"))
    {
      // Move to next stream. 
      // Deactivate rejected provider
      if (stream.provider())
      {
        stream.provider()->sessionTerminated(); // close corresponding media
        stream.setProvider( PDataProvider() );  // free provider
        SocketHeap::instance().freeSocketPair( stream.socket4() ); // close provider's socket ip4
        SocketHeap::instance().freeSocketPair( stream.socket6() ); // close provider's socket ip6
        s->mIceStack->removeStream( stream.iceInfo().mStreamId ); // remove stream from ice stack
      }
      continue;
    }

    // Try to parse SDP with ICE stack
    unsigned short remoteDefaultPort = remoteStream.port();

    // Extract remote ICE candidates vector
    const std::list<resip::Data> candidateList = remoteStream.getValues("candidate");
    if (candidateList.empty())
      iceAvailable = false;    

    std::vector<std::string> candidateVector;
    std::list<resip::Data>::const_iterator cit = candidateList.begin();
    for (; cit != candidateList.end(); ++cit)
      candidateVector.push_back(cit->c_str());
    
    if (remoteStream.exists("ice-pwd"))
      icePwd = remoteStream.getValues("ice-pwd").front().c_str();
    if (remoteStream.exists("ice-ufrag"))
      iceUfrag = remoteStream.getValues("ice-ufrag").front().c_str();

    // ICEBox::processSdpOffer() may install permissions on TURN peer - so call it anyway.
    // Also it can adjust number of ice components per stream;
    // Corresponding turn allocation will be removed in this case.
    try
    {
      if (!s->mIceStack->processSdpOffer(stream.iceInfo().mStreamId, candidateVector, remoteDefaultIP, remoteDefaultPort, mConfig[CONFIG_DEFERRELAYED].asBool()))
        iceAvailable = false;
    }
    catch(...)
    {
      iceAvailable = false;
    }

    // Process media description with provider
    if (stream.provider())
    {
      if (stream.provider()->processSdpOffer( remoteStream, Sdp_Answer ))
      {
        InternetAddress addr(remoteDefaultIP, remoteDefaultPort), addr2(remoteDefaultIP, remoteDefaultPort+1);

        // See if remote stream has "rtcp" or "rtcp-mux" attributes
        if (remoteStream.exists("rtcp"))
          addr2.setPort( StringHelper::toInt(remoteStream.getValues("rtcp").front().c_str(), remoteDefaultPort+1) );
        else
        if (remoteStream.exists("rtcp-mux"))
          addr2.setPort( remoteDefaultPort );
        stream.provider()->setDestinationAddress(RtpPair<InternetAddress>(addr, addr2));
        mediasupported = true;
      }
    }
  }
  
  // Save session handle
  if (!s->mInviteHandle.isValid())
    s->mInviteHandle = h;

  // Get ICE credentials
  if (icePwd.size() && iceUfrag.size())
  {
    s->mIceStack->setRemotePassword(icePwd);
    s->mIceStack->setRemoteUfrag(iceUfrag);
  }
  else
    iceAvailable = false;

  // Reject session if there is no media
  if (!mediasupported)
  {
    ICELogError(<< "Session " << s->mSessionId << ": no supported media. Ending the session.");
    h->end();
    return;
  }
  
  // Start connectivity checks now
  ICELogInfo(<< "Session " << s->mSessionId << ": attempt to check connectivity.");
  
  if (iceAvailable)
    s->mIceStack->checkConnectivity();
  else
  {
    if (!mConfig[CONFIG_RELAY].asBool())
      s->mIceStack->clear();
  }

  UInt64 version = sdp.session().origin().getVersion();
  s->mRemoteOriginVersion = version;
}

/// Called when an SDP offer is received - must send an answer soon after this
void UserAgent::onOffer(resip::InviteSessionHandle h, const resip::SipMessage& msg, const resip::SdpContents& sdp)
{
  PSession s = getUserSession(CAST2RESIPSESSION(h)->mSessionId);
  if (!s)
  {
    h->reject(488);
    return;
  }

  bool iceAvailable = true;

  ICELogInfo(<< "Session " << s->sessionId() << ": got offer.");
  
  // Check if sdp includes ICE ufrag/password
  std::string icePwd, iceUfrag;
  if (sdp.session().exists("ice-pwd"))
    icePwd = sdp.session().getValues("ice-pwd").front().c_str();
  if (sdp.session().exists("ice-ufrag"))
    iceUfrag = sdp.session().getValues("ice-ufrag").front().c_str();

  ice::Stack& ice = *s->mIceStack;

  UInt64 version = sdp.session().origin().getVersion();
  std::string remoteIp = sdp.session().connection().getAddress().c_str();
  int code;
  if ((UInt64)-1 == s->mRemoteOriginVersion)
  {
    code = s->processSdp(version, iceAvailable, icePwd, iceUfrag, remoteIp, sdp.session().media());
  }
  else
  if (version == s->mRemoteOriginVersion)
  {
    // Timer, answer with previous SDP
    //session->processTimer();
  }
  if (version == s->mRemoteOriginVersion+1)
  {
    // Updated offer. Here we must check if ICE has to be restarted.
    code = s->processSdp(version, iceAvailable, icePwd, iceUfrag, remoteIp, sdp.session().media());
  }
  s->mRemoteOriginVersion = version;
  
  if (code != 200)
  {
    h->reject(code);
    return;
  }

  // Save session handle
  if (!s->mInviteHandle.isValid())
    s->mInviteHandle = h;

  // Save reference to resip session
  s->mResipSession = CAST2RESIPSESSION(h);

  if (!s->mAcceptedByEngine)
  {
    // Do not call OnNewSession for this session in future
    s->mAcceptedByEngine = true;
    
    // Notify about new session request
    onNewSession(s);
  }
}

/// called when an Invite w/out SDP is sent, or any other context which
/// requires an SDP offer from the user
void UserAgent::onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called if an offer in a UPDATE or re-INVITE was rejected - not real
/// useful. A SipMessage is provided if one is available
void UserAgent::onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage* msg)
{
}

/// called when INFO message is received 
void UserAgent::onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called when response to INFO message is received 
void UserAgent::onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

void UserAgent::onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called when MESSAGE message is received 
void UserAgent::onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called when response to MESSAGE message is received 
void UserAgent::onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

void UserAgent::onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called when an REFER message is received.  The refer is accepted or
/// rejected using the server subscription. If the offer is accepted,
/// DialogUsageManager::makeInviteSessionFromRefer can be used to create an
/// InviteSession that will send notify messages using the ServerSubscription
void UserAgent::onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg)
{
}

void UserAgent::onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}


/// called when an REFER message receives a failure response 
void UserAgent::onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg)
{
}

/// called when an REFER message receives an accepted response 
void UserAgent::onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg)
{
}

void UserAgent::onDnsResult(const resip::DNSResult<resip::DnsHostRecord>& result)
{
  if (result.status == 0)
  {
    resip::Data foundAddress = result.records.front().host();
    ICELogInfo( << "Success to resolve STUN/TURN address to " << foundAddress.c_str());
    mConfig[CONFIG_STUNSERVER_IP] = std::string(foundAddress.c_str());
    onStart(0);
  }
  else
  {
    ICELogError( << "Failed to resolve STUN or TURN server IP address.");
    int startCode = mConfig[CONFIG_STUNSERVER_NAME].asStdString().empty() ? 0 : 503;
    onStart(startCode);
  }
}

void UserAgent::onDnsResult(const resip::DNSResult<resip::DnsAAAARecord>& result)
{
}

void UserAgent::onDnsResult(const resip::DNSResult<resip::DnsSrvRecord>& result)
{
}

void UserAgent::onDnsResult(const resip::DNSResult<resip::DnsNaptrRecord>& result)
{
  ;
}

void UserAgent::onDnsResult(const resip::DNSResult<resip::DnsCnameRecord>& result)
{
  ;
}
#pragma endregion


#pragma region Publication presence
void UserAgent::onSuccess(resip::ClientPublicationHandle h, const resip::SipMessage& status)
{
  resip::NameAddr from = status.header(resip::h_To);
  PAccount account = getAccount(from);
  if (account)
    account->mPublication = h;
  onPublicationSuccess(account);
}

void UserAgent::onRemove(resip::ClientPublicationHandle, const resip::SipMessage& status)
{
  resip::NameAddr to = status.header(resip::h_To);
  PAccount account = getAccount(to);
  if (account)
    account->mPublication = resip::ClientPublicationHandle();
  onPublicationTerminated(account, 0);
}

void UserAgent::onFailure(resip::ClientPublicationHandle, const resip::SipMessage& status)
{
  resip::NameAddr to = status.header(resip::h_To);
  PAccount account = getAccount(to);
  if (account)
    account->mPublication = resip::ClientPublicationHandle();
  onPublicationTerminated(account, status.header(resip::h_StatusLine).statusCode());
}

int UserAgent::onRequestRetry(resip::ClientPublicationHandle, int retrySeconds, const resip::SipMessage& status)
{
  return -1;
}



void UserAgent::onPublicationSuccess(PAccount account)
{
}

void UserAgent::onPublicationTerminated(PAccount account, int code)
{
}

#pragma endregion

#pragma region Subscriptions

void UserAgent::onClientObserverStart(PClientObserver observer)
{
}

void UserAgent::onClientObserverStop(PClientObserver observer, int code)
{
}

void UserAgent::onPresenceUpdate(PClientObserver observer, const std::string& peer, bool online, const std::string& content)
{
}


#pragma endregion

#pragma region SubscriptionHandler

void UserAgent::onNewSubscription(resip::ServerSubscriptionHandle h, const resip::SipMessage& sub)
{
  ResipSession* s = CAST2RESIPSESSION(h);
  
  // Get the event package name
  const char* event = sub.header(resip::h_Event).value().c_str();
  
  // Get the remote party
  const char* peer = sub.header(resip::h_From).uri().getAor().c_str();
  
  PAccount account = getAccount(sub.header(resip::h_From));
  PServerObserver so(new ServerObserver());
  so->mHandle = h;
  so->mPeer = peer;
  so->mPackage = event;
  so->mSessionId = s->sessionId();

  s->setRemoteAddress(peer);
  mServerObserverMap[so->mSessionId] = so;
  onServerObserverStart(so);
}

void UserAgent::onTerminated(resip::ServerSubscriptionHandle h)
{
  if (!h.isValid())
    return;
  ResipSession* s = CAST2RESIPSESSION(h);
  if (!s)
    return;

  ServerObserverMap::iterator observerIter = mServerObserverMap.find(s->sessionId());
  if (observerIter != mServerObserverMap.end())
  {
    onServerObserverStop(observerIter->second, 0);
    mServerObserverMap.erase(observerIter);
  }
}

void UserAgent::onServerObserverStart(PServerObserver observer)
{
}

void UserAgent::onServerObserverStop(PServerObserver observer, int code)
{
}

void UserAgent::onUpdate(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify)
{
  if (!h.isValid())
    return;
  
  ResipSession* s = CAST2RESIPSESSION(h);
  if (!s)
    return;
  ClientObserverMap::iterator observerIter = mClientObserverMap.find(s->sessionId());
  if (observerIter == mClientObserverMap.end())
  {
    h->rejectUpdate();
    h->end();
    return;
  }
  PClientObserver observer = observerIter->second;

  std::vector<resip::Data> availableContacts;
  h->acceptUpdate();

  // Find "from" header
  resip::Uri from = notify.header(resip::h_From).uri();
  
  // Find content
  resip::Contents* contents = notify.getContents();
  if (contents)
  {
    // Check if pidf
    if (resip::Pidf* pidf = dynamic_cast<resip::Pidf*>(contents))
    {
      resip::Data body = pidf->getBodyData();
      bool online = pidf->getSimpleStatus(&body);
      onPresenceUpdate(observer, observer->peer(), online, std::string(body.c_str(), body.size()));
    }
    else
    if (resip::MultipartRelatedContents* mr = dynamic_cast<resip::MultipartRelatedContents*>(contents))
    {
      resip::MultipartRelatedContents::Parts& parts = mr->parts();
      for( resip::MultipartRelatedContents::Parts::const_iterator i = parts.begin(); i != parts.end();  ++i)
      {
        resip::Contents* c = *i;  
        assert( c );
        resip::Mime m = c->getType();
        if (resip::Rlmi* rlmi = dynamic_cast<resip::Rlmi*>(c))
        {
          resip::Data d = rlmi->getBodyData();
          // Workaround for XMLCursor bug
          if (d.find(resip::Data("<resource")) == resip::Data::npos)
            continue;

          resip::ParseBuffer b(d);
          resip::XMLCursor c(b);
          resip::Data tag = c.getTag();
          if (tag != "list")
          {
            ICELogError( << "Failed to find <list> tag in rlmi");
          }
          else
          {
            for (bool hasRecord = c.firstChild(); hasRecord; hasRecord = c.nextSibling())
            {
              if (c.getTag() != "resource")
              {
                ICELogError( << "Failed to find <resource> tag in rlmi");
              }
              else
              {
                // Check if c has <instance>

                resip::XMLCursor::AttributeMap::const_iterator attrIter = c.getAttributes().find("uri");
                if (attrIter != c.getAttributes().end())
                {
                  // Save uri
                  resip::Data uri = attrIter->second;

                  // Check if there is <instance> tag
                  bool instance = false;
                  for (bool hasChild = c.firstChild(); hasChild; hasChild = c.nextSibling())
                    instance |= c.getTag() == "instance";
                  c.parent();

                  // Save result
                  if (instance)
                    availableContacts.push_back( attrIter->second );
                }
              }
            }
          }
        }
        else
        if (resip::Pidf* pidf = dynamic_cast<resip::Pidf*>(c))
        {
          resip::Data body = pidf->getBodyData();
          bool online = pidf->getSimpleStatus(&body);
          resip::Data entity = pidf->getEntity().getAorNoPort();
          if (entity.find("sip:") == resip::Data::npos && entity.find("sips:") == resip::Data::npos)
            entity = resip::Data("sip:") + entity;
          onPresenceUpdate(observer, entity.c_str(), online, std::string(body.c_str(), body.size()));
          
          // Drop corresponding record from availableContacts
          std::vector<resip::Data>::iterator ci = std::find(availableContacts.begin(), availableContacts.end(), entity);
          if (ci != availableContacts.end())
            availableContacts.erase(ci);
        }
      }
    }
  }
  
  for (unsigned i=0; i<availableContacts.size(); i++)
    onPresenceUpdate(observer, availableContacts[i].c_str(), false, std::string());
}

void UserAgent::onUpdatePending(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
  onUpdate(h, notify);
}

void UserAgent::onUpdateActive(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
  onUpdate(h, notify);
}

void UserAgent::onUpdateExtension(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify, bool outOfOrder)
{
  onUpdate(h, notify);
}

int UserAgent::onRequestRetry(resip::ClientSubscriptionHandle, int retrySeconds, const resip::SipMessage& notify)
{
  return -1;
}

//subscription can be ended through a notify or a failure response.
void UserAgent::onTerminated(resip::ClientSubscriptionHandle h, const resip::SipMessage* msg)
{
  // TODO - check for refer notication
  if (!h.isValid() || !msg)
    return;
  
  ResipSession* rs = CAST2RESIPSESSION(h);
  if (!rs)
    return;
  
  // Find the response code if available
  int code = 0;
  if (msg->isResponse())
    code = msg->header(resip::h_StatusLine).statusCode();
  
  // Remove subscription from list, call terminated event 
  rs->runTerminatedEvent(ResipSession::Type_Subscription, code, 0);
}

void UserAgent::onNewSubscription(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify)
{
  if (!h.isValid())
    return;

  // Find dialog set
  ResipSession* s = CAST2RESIPSESSION(h);
  if (!s)
    return;
  
  if (!s->mOnWatchingStartSent)
  {
    s->mOnWatchingStartSent = true;
    ClientObserverMap::iterator observerIter = mClientObserverMap.find(s->sessionId());
    if (observerIter != mClientObserverMap.end())
      onClientObserverStart(observerIter->second);
  }
}

/// called to allow app to adorn a message.
void UserAgent::onReadyToSend(resip::ClientSubscriptionHandle, resip::SipMessage& msg)
{
}

void UserAgent::onNotifyNotReceived(resip::ClientSubscriptionHandle)
{
}

/// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
/// errors, or missing CRLF keep alives pong responses from the server.
//  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
/// supports RFC5626 (outbound).
/// Default implementation is to re-form the subscription using a new flow
void UserAgent::onFlowTerminated(resip::ClientSubscriptionHandle)
{
}
#pragma endregion

#pragma region PagerHandler
void UserAgent::onSuccess(resip::ClientPagerMessageHandle h, const resip::SipMessage& status)
{
  if (!h.isValid())
    return;
  ResipSession* s = CAST2RESIPSESSION(h);
  if (!s)
    return;
  onMessageSent(getAccount(status.header(resip::h_From)), s->sessionId(), s->remoteAddress(), s->tag());
}

void UserAgent::onFailure(resip::ClientPagerMessageHandle h, const resip::SipMessage& status, std::unique_ptr<resip::Contents> contents)
{
  if (!h.isValid())
    return;
  ResipSession* s = CAST2RESIPSESSION(h);
  if (!s)
    return;
  onMessageFailed(getAccount(status.header(resip::h_From)), s->sessionId(), s->remoteAddress(), status.header(resip::h_StatusLine).statusCode(), s->tag());
}

void UserAgent::onMessageArrived(resip::ServerPagerMessageHandle h, const resip::SipMessage& message)
{
  if (!h.isValid())
    return;
  resip::NameAddr from = message.header(resip::h_From);
  std::string peer(from.uri().getAor().c_str());
  PAccount account = getAccount(from);
  h->send(h->accept());
  resip::Contents* c = message.getContents();
  if (!c)
    onMessageArrived(account, peer, NULL, 0);
  else
  {
    resip::Data d = c->getBodyData();
    onMessageArrived(account, peer, d.c_str(), d.size());
  }
}

void UserAgent::updateInterfaceList()
{
  //ICEImpl::ICENetworkHelper::instance().reload();
}

void UserAgent::onMessageArrived(PAccount /*account*/, const std::string& /*peer*/, const void* /*ptr*/, unsigned /*length*/)
{
}

void UserAgent::onMessageFailed(PAccount /*account*/, int /*id*/, const std::string& /*peer*/, int /*code*/, void* /*tag*/)
{
}

void UserAgent::onMessageSent(PAccount /*account*/, int /*id*/, const std::string& /*peer*/, void* /*tag*/)
{
}

VariantMap& UserAgent::config()
{
  return mConfig;
}

static void splitToHeaders(resip::Data& content, std::vector<resip::Data>& output)
{
  resip::Data::size_type startLine = 0, endLine = content.find("\r\n");
  while (endLine != resip::Data::npos)
  {
    output.push_back(content.substr(startLine, endLine));
    
    startLine = endLine + 2;
    endLine = content.find("\r\n", startLine);
  }
  if (0 == startLine)
    output.push_back(content);
}

static bool parseHeader(resip::Data& input, resip::Data& name, resip::Data& value)
{
  resip::Data::size_type p = input.find(":");
  if (p == resip::Data::npos)
  {
    name = input;
    value.clear();
  }
  else
  {
    name = input.substr(0, p);
    value = input.substr(p+1, input.size() - p - 1);
    
    // Trim leading whitespace
    if (value.size())
    {
      if (value.at(0) == ' ')
        value = value.substr(1, value.size()-1);
    }
  }
  return true;
}

void UserAgent::onSipMessage(int flow, const char* msg, unsigned int length, const sockaddr* addr, unsigned int addrlen)
{
  std::string d(msg, length);
  ice::NetworkAddress address(*addr, addrlen);
  std::string addressText = address.toStdString();

  switch (flow)
  {
  case resip::InternalTransport::TransportLogger::Flow_Received:
    ICELogDebug(<< "Received from " << addressText << ":" << "\n"
                        << StringHelper::prefixLines(d, "--->"));
    break;

  case resip::InternalTransport::TransportLogger::Flow_Sent:
    ICELogDebug(<< "Sent to " << addressText << "\n" << StringHelper::prefixLines(d, "<---"));
    break;
  }
}

PSession UserAgent::getUserSession(int sessionId)
{
  SessionMap::iterator sessionIter = mSessionMap.find(sessionId);
  if (sessionIter != mSessionMap.end())
    return sessionIter->second;
  else
    return PSession();
}

PAccount UserAgent::getAccount(const resip::NameAddr& myAddr)
{
  PAccount acc;
  for (AccountSet::iterator accountIter = mAccountSet.begin(); accountIter != mAccountSet.end() && !acc; accountIter++)
    if ((*accountIter)->isResponsibleFor(myAddr))
      acc = *accountIter;

  return acc;
}

PAccount UserAgent::getAccount(Account* account)
{
  PAccount acc;
  for (AccountSet::iterator accountIter = mAccountSet.begin(); accountIter != mAccountSet.end() && !acc; accountIter++)
    if (accountIter->get() == account)
      acc = *accountIter;

  return acc;
}

PAccount UserAgent::getAccount(int sessionId)
{
  auto profileIter = std::find_if(mAccountSet.begin(), mAccountSet.end(), [=](const AccountSet::value_type& v) {if (v->mRegistration) return v->mRegistration->sessionId() == sessionId; else return false;});
  return (profileIter != mAccountSet.end()) ? *profileIter : PAccount();
}

#pragma endregion
