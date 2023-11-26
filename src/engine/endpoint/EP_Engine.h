/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ENGINE_H
#define __ENGINE_H

#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/InternalTransport.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientInviteSession.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ServerInviteSession.hxx"
#include "resip/dum/ServerOutOfDialogReq.hxx"
#include "resip/dum/OutOfDialogHandler.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialogSetFactory.hxx"
#include "resip/dum/ClientPublication.hxx"
#include "resip/dum/ClientSubscription.hxx"
#include "resip/dum/SubscriptionHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/dum/PublicationHandler.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/DnsResult.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/dns/QueryTypes.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "../ice/ICEBox.h"
#include "../ice/ICETime.h"
#include <sstream>
#include <time.h>
#include "../engine_config.h"
#include "EP_Session.h"
#include "EP_Observer.h"
#include "EP_DataProvider.h"
#include "../helper/HL_VariantMap.h"
#include "../helper/HL_SocketHeap.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_ByteBuffer.h"
#include "../media/MT_Stream.h"

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace std;
enum
{
    TransportType_Any,
    TransportType_Udp,
    TransportType_Tcp,
    TransportType_Tls
};

enum
{
  CONFIG_IPV4 = 0,            // Use IP4
  CONFIG_IPV6,                // Use IP6.
  CONFIG_USERNAME,            // Username. String value.
  CONFIG_DOMAIN,              // Domain. String value.
  CONFIG_PASSWORD,            // Password. String value.
  CONFIG_RINSTANCE,           // Determines if SIP rinstance field has to be used during registration. Boolean value.
  CONFIG_INSTANCE_ID,         // Instance id. It is alternative option to rinstance.
  CONFIG_DISPLAYNAME,         // Optional user display name. String value.
  CONFIG_DOMAINPORT,          // Optional domain port number. Integer value.
  CONFIG_REGISTERDURATION,    // Wanted duration for registration. Integer value. It is MANDATORY value.
  CONFIG_RPORT,               // Use SIP rport field. Recommended to set it to true. Boolean value.
  CONFIG_KEEPALIVETIME,       // Interval between UDP keep-alive messages. Boolean value.
  CONFIG_RELAY,               // Sets if TURN server must be used instead of STUN. Boolean value.
  CONFIG_ICETIMEOUT,          // Optional timeout for ICE connectivity checks and candidate gathering. Integer value.
  CONFIG_ICEUSERNAME,	        // Optional username for TURN server. String value.
  CONFIG_ICEPASSWORD,         // Optional password for TURN server. String value.
  CONFIG_SIPS,                // Marks if account credentials are sips: scheme. Boolean value.
  CONFIG_STUNSERVER_IP,       // Optional IP address of STUN/TURN server. String value. It is better to use CONFIG_STUNSERVER_NAME.
  CONFIG_STUNSERVER_NAME,     // Host name of STUN/TURN server. stun.xten.com for example. String value.
  CONFIG_STUNSERVER_PORT,     // Port number of STUN/TURN server. Integer value.
  CONFIG_USERAGENT,           // Name of user agent in SIP headers. String value.
  CONFIG_ICEREQUIRED,         // ICE MUST be present in remote peer offers and answers. Boolean value.
  CONFIG_TRANSPORT,           // 0 - all transports, 1 - UDP, 2 - TCP, 3 - TLS,
  CONFIG_SUBSCRIPTION_TIME,   // Subscription time (in seconds)
  CONFIG_SUBSCRIPTION_REFRESHTIME, // Refresh interval for subscriptions
  CONFIG_DNS_CACHE_TIME,       // DNS cache time; default is 86400 seconds
  CONFIG_PRESENCE_ID,         // Tuple ID used in presence publishing; determines source device
  CONFIG_ROOTCERT,            // Additional root cert in PEM format; string.
  CONFIG_CACHECREDENTIALS,    // Attempt to cache credentials that comes in response from PBX. Use them when possible to reduce number of steps of SIP transaction
  CONFIG_RTCP_ATTR,           // Use "rtcp" attribute in sdp. Default value is true.
  CONFIG_MULTIPLEXING,        // Do rtp/rtcp multiplexing
  CONFIG_DEFERRELAYED,        // Defer relayed media path
  CONFIG_PROXY,               // Proxy host name or IP address
  CONFIG_PROXYPORT,           // Proxy port number
  CONFIG_CODEC_PRIORITY,      // Another VariantMap with codec priorities,
  CONFIG_ACCOUNT,             // VariantMap with account configuration
  CONFIG_EXTERNALIP,          // Use external/public IP in outgoing requests
  CONFIG_OWN_DNS,             // Use predefined DNS servers
  CONFIG_REGID                // reg-id value from RFC5626
};

// Conntype parameter for OnSessionEstablished event
enum
{
  EV_SIP = 1,
  EV_ICE = 2
};

class UserAgent;

// Define a type for asynchronous requests to user agent
class SIPAction
{
public:
  virtual void Run(UserAgent& ua) = 0;
};

typedef std::vector<SIPAction*> SIPActionVector;

// Session termination reason
enum 
{
  Error,
  Timeout, 
  Replaced,
  LocalBye,
  RemoteBye,
  LocalCancel,
  RemoteCancel,
  Rejected, //Only as UAS, UAC has distinct onFailure callback
  Referred
};

class UserAgent:  public resip::ClientRegistrationHandler, 
                  public resip::InviteSessionHandler, 
                  public resip::DumShutdownHandler, 
                  public resip::ExternalLogger, 
                  public resip::DnsResultSink,
                  public resip::ClientSubscriptionHandler, 
                  public resip::ServerSubscriptionHandler,
                  public resip::ClientPagerMessageHandler, 
                  public resip::ServerPagerMessageHandler,
                  public resip::ClientPublicationHandler
                  //public resip::InternalTransport::TransportLogger
{
  friend class Account;
  friend class Session;
  friend class ResipSession;
  friend class NATDecorator;
  friend class WatcherQueue;
public:
  /* Compares two sip addresses. Returns true if they represent the same entity - user and domain are the same. Otherwise returns false. */
  static bool compareSipAddresses(const std::string& sip1, const std::string& sip2);
  static std::string formatSipAddress(const std::string& sip);
  static bool isSipAddressValid(const std::string& sip);
  struct SipAddress
  {
    bool mValid;
    std::string mScheme;
    std::string mUsername;
    std::string mDomain;
    std::string mDisplayname;
  };

  static SipAddress parseSipAddress(const std::string& sip);

  UserAgent();
  virtual ~UserAgent();
  
  /* Brings user agent online. Basically it creates a signalling socket(s).
     This is asynchronous method. */
  void start();
  
  /* Shutdowns user agent. It closes all sessions, tries to unregister from server and disconnects from it. 
     This is asynchronous method. onStop() event will be called later */
  void shutdown();
  
  /* Emergency stop. Please always call shutdown() before this. Kills registration, sessions & presence - everything. onStop() is called in context of this method. */
  void stop();

  /* Checks if user agent is active (started). */
  bool active();
  
  /* Used to refresh existing registration(s), publication, subscriptions. */
  void refresh();

  /* Runs sip & ice stacks. Event handlers are called in its context. */
  void process();
  
  /* Adds root cert in PEM format. Usable after start() call. */
  void addRootCert(const ByteBuffer& data);

  PAccount createAccount(PVariantMap config);
  void deleteAccount(PAccount account);

  /* Creates session. Returns session ID. */
  PSession createSession(PAccount account);
  
  // Must be called when IP interface list is changed
  void updateInterfaceList();

  // Called on new incoming session; providers shoukld
  virtual PDataProvider onProviderNeeded(const std::string& name) = 0;

  // Called on new session offer
  virtual void onNewSession(PSession s) = 0;
  
  // Called when session is terminated
  virtual void onSessionTerminated(PSession s, int responsecode, int reason) = 0;
  
  // Called when session is established ok i.e. after all ICE signalling is finished
  // Conntype is type of establish event - EV_SIP or EV_ICE
  virtual void onSessionEstablished(PSession s, int conntype, const RtpPair<InternetAddress>& p) = 0;

  // Called when client session gets
  virtual void onSessionProvisional(PSession s, int code) = 0;

  // Called when user agent started
  virtual void onStart(int errorcode) = 0;
  
  // Called when user agent stopped
  virtual void onStop() = 0;

  // Called when account registered
  virtual void onAccountStart(PAccount account) = 0;

  // Called when account removed or failed (non zero error code)
  virtual void onAccountStop(PAccount account, int error) = 0;

  // Called when connectivity checks failed.
  virtual void onConnectivityFailed(PSession s) = 0;

  // Called when new candidate is gathered
  virtual void onCandidateGathered(PSession s, const char* address);
  
  // Called when network change detected
  virtual void onNetworkChange(PSession s) = 0;

  // Called when all candidates are gathered
  virtual void onGathered(PSession s);

  // Called when new connectivity check is finished
  virtual void onCheckFinished(PSession s, const char* description);
  
  // Called when log message must be recorded
  virtual void onLog(const char* msg);
  
  // Called when problem with SIP connection(s) detected
  virtual void onSipConnectionFailed() = 0;

  // Subscribe/publish presence methods
  virtual void onPublicationSuccess(PAccount acc);
  virtual void onPublicationTerminated(PAccount acc, int code);
  virtual void onClientObserverStart(PClientObserver observer);
  virtual void onServerObserverStart(PServerObserver observer);
  virtual void onClientObserverStop(PClientObserver observer, int code);
  virtual void onServerObserverStop(PServerObserver observer, int code);

  virtual void onPresenceUpdate(PClientObserver observer, const std::string& peer, bool online, const std::string& content);
  virtual void onMessageArrived(PAccount account, const std::string& peer, const void* ptr, unsigned length);
  virtual void onMessageFailed(PAccount account, int id, const std::string& peer, int code, void* tag);
  virtual void onMessageSent(PAccount account, int id, const std::string& peer, void* tag);
  
  // Configuration methods
  VariantMap& config();

public:
  // InviteSessionHandler implementation
#pragma region InviteSessionHandler implementation
  /// called when an initial INVITE or the intial response to an outoing invite  
  virtual void onNewSession(resip::ClientInviteSessionHandle, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg) override;
  virtual void onNewSession(resip::ServerInviteSessionHandle, resip::InviteSession::OfferAnswerType oat, const resip::SipMessage& msg) override;

  /// Received a failure response from UAS
  virtual void onFailure(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override;
      
  /// called when an in-dialog provisional response is received that contains an SDP body
  virtual void onEarlyMedia(resip::ClientInviteSessionHandle, const resip::SipMessage&, const resip::SdpContents&) override;

  /// called when dialog enters the Early state - typically after getting 18x
  virtual void onProvisional(resip::ClientInviteSessionHandle, const resip::SipMessage&) override;

  /// called when a dialog initiated as a UAC enters the connected state
  virtual void onConnected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when a dialog initiated as a UAS enters the connected state
  virtual void onConnected(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  virtual void onTerminated(resip::InviteSessionHandle, resip::InviteSessionHandler::TerminatedReason reason, const resip::SipMessage* related=0) override;

  /// called when a fork that was created through a 1xx never receives a 2xx
  /// because another fork answered and this fork was canceled by a proxy. 
  virtual void onForkDestroyed(resip::ClientInviteSessionHandle) override;

  /// called when a 3xx with valid targets is encountered in an early dialog     
  /// This is different then getting a 3xx in onTerminated, as another
  /// request will be attempted, so the DialogSet will not be destroyed.
  /// Basically an onTermintated that conveys more information.
  /// checking for 3xx respones in onTerminated will not work as there may
  /// be no valid targets.
  virtual void onRedirected(resip::ClientInviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when an SDP answer is received - has nothing to do with user
  /// answering the call 
  virtual void onAnswer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&) override;

  /// called when an SDP offer is received - must send an answer soon after this
  virtual void onOffer(resip::InviteSessionHandle, const resip::SipMessage& msg, const resip::SdpContents&) override;

  /// called when an Invite w/out SDP is sent, or any other context which
  /// requires an SDP offer from the user
  virtual void onOfferRequired(resip::InviteSessionHandle, const resip::SipMessage& msg) override;
  
  /// called if an offer in a UPDATE or re-INVITE was rejected - not real
  /// useful. A SipMessage is provided if one is available
  virtual void onOfferRejected(resip::InviteSessionHandle, const resip::SipMessage* msg) override;
  
  /// called when INFO message is received 
  virtual void onInfo(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when response to INFO message is received 
  virtual void onInfoSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override;
  virtual void onInfoFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when MESSAGE message is received 
  virtual void onMessage(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when response to MESSAGE message is received 
  virtual void onMessageSuccess(resip::InviteSessionHandle, const resip::SipMessage& msg) override;
  virtual void onMessageFailure(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when an REFER message is received.  The refer is accepted or
  /// rejected using the server subscription. If the offer is accepted,
  /// DialogUsageManager::makeInviteSessionFromRefer can be used to create an
  /// InviteSession that will send notify messages using the ServerSubscription
  virtual void onRefer(resip::InviteSessionHandle, resip::ServerSubscriptionHandle, const resip::SipMessage& msg) override;

  virtual void onReferNoSub(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when an REFER message receives a failure response 
  virtual void onReferRejected(resip::InviteSessionHandle, const resip::SipMessage& msg) override;

  /// called when an REFER message receives an accepted response 
  virtual void onReferAccepted(resip::InviteSessionHandle, resip::ClientSubscriptionHandle, const resip::SipMessage& msg) override;
#pragma endregion

  // ClientRegistrationHandler implementation
#pragma region ClientRegistrationHandler implementation
  /// Called when registraion succeeds or each time it is sucessfully
  /// refreshed. 
  void onSuccess(resip::ClientRegistrationHandle, const resip::SipMessage& response) override;

  // Called when all of my bindings have been removed
  void onRemoved(resip::ClientRegistrationHandle, const resip::SipMessage& response) override;
  
  /// call on Retry-After failure. 
  /// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
  int onRequestRetry(resip::ClientRegistrationHandle, int retrySeconds, const resip::SipMessage& response) override;
  
  /// Called if registration fails, usage will be destroyed (unless a 
  /// Registration retry interval is enabled in the Profile)
  void onFailure(resip::ClientRegistrationHandle, const resip::SipMessage& response) override;

#pragma endregion

#pragma region ExternalLogger implementation
   /** return true to also do default logging, false to suppress default logging. */
  virtual bool operator()(resip::Log::Level level,
                          const resip::Subsystem& subsystem,
                          const resip::Data& appName,
                          const char* file,
                          int line,
                          const resip::Data& message,
                          const resip::Data& messageWithHeaders,
                          const resip::Data& instanceName) override;
#pragma endregion

#pragma region DnsResultSink implementation

    virtual void onDnsResult(const resip::DNSResult<resip::DnsHostRecord>&) override;
    virtual void onDnsResult(const resip::DNSResult<resip::DnsAAAARecord>&) override;
    virtual void onDnsResult(const resip::DNSResult<resip::DnsSrvRecord>&) override;
    virtual void onDnsResult(const resip::DNSResult<resip::DnsNaptrRecord>&) override;
    virtual void onDnsResult(const resip::DNSResult<resip::DnsCnameRecord>&) override;

#pragma endregion

#pragma region TransportLogger implementation
    void onSipMessage(int flow, const char* msg, unsigned int length, const sockaddr* addr, unsigned int addrlen);
#pragma  endregion

#pragma region ClientPublicationHandler
  void onSuccess(resip::ClientPublicationHandle, const resip::SipMessage& status) override;
  void onRemove(resip::ClientPublicationHandle, const resip::SipMessage& status) override;
  void onFailure(resip::ClientPublicationHandle, const resip::SipMessage& status) override;
  int onRequestRetry(resip::ClientPublicationHandle, int retrySeconds, const resip::SipMessage& status) override;
#pragma endregion

#pragma region SubscriptionHandler
  void onUpdate(resip::ClientSubscriptionHandle h, const resip::SipMessage& notify);
  void onUpdatePending(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder) override;
  void onUpdateActive(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder) override;
  
  //unknown Subscription-State value
  void onUpdateExtension(resip::ClientSubscriptionHandle, const resip::SipMessage& notify, bool outOfOrder) override;
  int onRequestRetry(resip::ClientSubscriptionHandle, int retrySeconds, const resip::SipMessage& notify) override;
      
  //subscription can be ended through a notify or a failure response.
  void onTerminated(resip::ClientSubscriptionHandle, const resip::SipMessage* msg) override;
  //not sure if this has any value.
  void onNewSubscription(resip::ClientSubscriptionHandle, const resip::SipMessage& notify) override;

  /// called to allow app to adorn a message.
  void onReadyToSend(resip::ClientSubscriptionHandle, resip::SipMessage& msg) override;
  void onNotifyNotReceived(resip::ClientSubscriptionHandle) override;

  /// Called when a TCP or TLS flow to the server has terminated.  This can be caused by socket
  /// errors, or missing CRLF keep alives pong responses from the server.
  //  Called only if clientOutbound is enabled on the UserProfile and the first hop server 
  /// supports RFC5626 (outbound).
  /// Default implementation is to re-form the subscription using a new flow
  void onFlowTerminated(resip::ClientSubscriptionHandle) override;
  void onNewSubscription(resip::ServerSubscriptionHandle, const resip::SipMessage& sub) override;
  void onTerminated(resip::ServerSubscriptionHandle) override;
#pragma endregion

#pragma region PagerHandler
  void onSuccess(resip::ClientPagerMessageHandle, const resip::SipMessage& status) override;
  void onFailure(resip::ClientPagerMessageHandle, const resip::SipMessage& status, std::unique_ptr<resip::Contents> contents) override;
  void onMessageArrived(resip::ServerPagerMessageHandle, const resip::SipMessage& message) override;
#pragma endregion

  void onDumCanBeDeleted() override;
protected:
  // Mutex to protect this instance
  Mutex mGuard;

  // Smart pointer to resiprocate's master profile instance. The stack configuration holds here.
  std::shared_ptr<resip::MasterProfile> mProfile;
  
  // Resiprocate's SIP stack object pointer
  resip::SipStack* mStack;
  
  // Resiprocate's dialog usage manager object pointer
  resip::DialogUsageManager* mDum;
  
  // List of available transports. They are owned by SipStack - so there is no need to delete instances in UserAgent.
  std::vector<resip::InternalTransport*> mTransportList;

  typedef std::map<int, PSession> SessionMap;
  
  // Session's map
  SessionMap mSessionMap;

  // Used configuration
  VariantMap mConfig;

  // Action vector
  SIPActionVector mActionVector;
  
  typedef std::map<int, PClientObserver> ClientObserverMap;
  ClientObserverMap mClientObserverMap;

  typedef std::map<int, PServerObserver> ServerObserverMap;
  ServerObserverMap mServerObserverMap;
  
  typedef std::set<PAccount> AccountSet;
  AccountSet mAccountSet;

  // Constructs and sends INVITE to remote peer. Remote peer address is stored inside session object.
  void sendOffer(Session* session);
  void internalStopSession(Session& session);
  void processWatchingList();
  bool handleMultipartRelatedNotify(const resip::SipMessage& notify);

  PSession getUserSession(int sessionId);
  PAccount getAccount(const resip::NameAddr& myAddr);
  PAccount getAccount(Account* account);
  PAccount getAccount(int sessionId);
};
#endif
