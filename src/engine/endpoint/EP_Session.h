/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SESSION_H
#define __SESSION_H

#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/SipStack.hxx"
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
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/AtomicCounter.hxx"

#include "../ice/ICEBox.h"

#include <sstream>
#include <time.h>

#include "../config.h"
#include "EP_Session.h"
#include "EP_Account.h"
#include "EP_DataProvider.h"
#include "EP_AudioProvider.h"
#include "../helper/HL_VariantMap.h"
#include "../helper/HL_SocketHeap.h"

using namespace std;

class UserAgent;
class ResipSession;

enum SessionInfo
{
  SessionInfo_RemoteSipAddress,     // remote sip address
  SessionInfo_ReceivedTraffic,      // amount of received traffic in session in bytes
  SessionInfo_SentTraffic,          // amount of sent traffic in session in bytes
  SessionInfo_PacketLoss,           // lost packets counter; returns number of 1/1000 fractions (0.1%)
  SessionInfo_AudioPeer,            // remote peer rtp address in text
  SessionInfo_AudioCodec,           // selected audio codec as text
  SessionInfo_DtmfInterface,        // Pointer to DtmfQueue class; returned as void*
  SessionInfo_IceState,
  SessionInfo_NetworkMos,
  SessionInfo_PvqaMos,
  SessionInfo_PvqaReport,
  SessionInfo_SentRtp,
  SessionInfo_SentRtcp,
  SessionInfo_ReceivedRtp,
  SessionInfo_ReceivedRtcp,
  SessionInfo_LostRtp,
  SessionInfo_Duration,
  SessionInfo_Jitter,
  SessionInfo_Rtt,
  SessionInfo_BitrateSwitchCounter, // It is for AMR codecs only
  SessionInfo_RemotePeer,
  SessionInfo_SSRC
};


class Session :
                public SocketSink,
                public ice::StageHandler
{
public:
  class Command
  {
  public:
    virtual void run(Session& s) = 0;
  };

  // Describes ice stream/component
  struct IceInfo
  {
    IceInfo()
      :mStreamId(-1)
    {
      mPort4 = mPort6 = 0;
      mComponentId.mRtp = mComponentId.mRtcp = -1;
    }

    RtpPair<int> mComponentId;
    int mStreamId;
    unsigned short mPort4;
    unsigned short mPort6;
  };

  // Describes media stream (audio/video) in session
  class Stream
  {
  public:
    Stream();
    ~Stream();

    void setProvider(PDataProvider provider);
    PDataProvider provider();

    void setSocket4(const RtpPair<PDatagramSocket>& socket);
    RtpPair<PDatagramSocket>& socket4();

    void setSocket6(const RtpPair<PDatagramSocket>& socket);
    RtpPair<PDatagramSocket>& socket6();

    void setIceInfo(const IceInfo& info);
    IceInfo iceInfo() const;

    // rtcpAttr/rtcpMuxAttr signals about corresponding sip attribute in offer/answer from remote peer
    bool rtcpAttr() const;
    void setRtcpAttr(bool value);

    bool rtcpMuxAttr() const;
    void setRtcpMuxAttr(bool value);

  protected:
    // Provider for corresponding stream
    PDataProvider   mProvider;

    // Socket for stream
    RtpPair<PDatagramSocket>  mSocket4, mSocket6;

    bool              mRtcpAttr;
    bool              mRtcpMuxAttr;
    IceInfo           mIceInfo;
  };

  Session(PAccount account);
  virtual ~Session();

  // Starts call to specified peer
  void start(const std::string& peer);

  // Stops call
  void stop();

  // Accepts call
  void accept();

  // Rejects call
  void reject(int code);

  enum class InfoOptions
  {
      Standard = 0,
      Detailed = 1,
  };

  void getSessionInfo(InfoOptions options, VariantMap& result);

  // Returns integer identifier of the session; it is unique amongst all session in application
  int id() const;

  // Returns owning account
  PAccount account();

  typedef std::map<std::string, std::string> UserHeaders;
  void setUserHeaders(const UserHeaders& headers);

  // Called when new media data are available for this session
  void onReceivedData(PDatagramSocket socket, InternetAddress& src, const void* receivedPtr, unsigned receivedSize);

  // Called when new candidate is gathered
  void onCandidateGathered(ice::Stack* stack, void* tag, const char* address);

  // Called when connectivity check is finished
  void onCheckFinished(ice::Stack* stack, void* tag, const char* checkDescription);

  // Called when ICE candidates are gathered - with success or timeout.
  void onGathered(ice::Stack* stack, void* tag);

  // Called when ICE connectivity check is good at least for one of required streams
  void onSuccess(ice::Stack* stack, void* tag);

  // Called when ICE connectivity check is failed for all of required streams
  void onFailed(ice::Stack* stack, void* tag);

  // Called when ICE stack detects network change during the call
  void onNetworkChange(ice::Stack* stack, void* tag);

  // Fills SDP according to ICE and provider's data
  void buildSdp(resip::SdpContents& sdp, SdpDirection sdpDirection);

  // Searches provider by its local port number
  PDataProvider findProviderByPort(int family, unsigned short port);

  // Add provider to internal list
  void addProvider(PDataProvider provider);
  PDataProvider providerAt(int index);
  int getProviderCount();

  void setUserAgent(UserAgent* agent);
  UserAgent* userAgent();

  // Pauses and resumes all providers; updates states
  void pause();
  void resume();
  void refreshMediaPath();

  // Processes new sdp from offer. Returns response code (200 is ok, 488 bad codec, 503 internal error).
  int processSdp(UInt64 version, bool iceAvailable, std::string icePwd, std::string iceUfrag,
    std::string remoteIp, const resip::SdpContents::Session::MediumContainer& media);

  // Session ID
  int                  mSessionId;

  // Media streams collection
  std::vector<Stream>  mStreamList;

  // Smart pointer to ICE stack. Actually stack is created in CreateICEStack() method
  resip::SharedPtr<ice::Stack>    mIceStack;

  // Pointer to owner user agent instance
  UserAgent*           mUserAgent;

  // Remote peer SIP address
  resip::NameAddr      mRemotePeer;

  // Mutex to protect this instance
  Mutex                mGuard;

  // SDP's origin version for sending
  int                  mOriginVersion;
  UInt64               mRemoteOriginVersion;

  // SDP's session version
  int                  mSessionVersion;

  // Marks if this session does not need OnNewSession event
  bool                 mAcceptedByEngine;
  bool                 mAcceptedByUser;

  // Invite session handle
  resip::InviteSessionHandle  mInviteHandle;

  // Dialog set object pointer
  ResipSession* mResipSession;

  // Reference counter
  int mRefCount;

  enum
  {
     Initiator = 1,
     Acceptor = 2
  };

   // Specifies session role - caller (Initiator) or callee (Acceptor)
   volatile int         mRole;

   // Marks if candidates are gather already
   volatile bool        mGatheredCandidates;

   // Marks if OnTerminated event was called already on session
   volatile bool        mTerminated;

   // User friend remote peer's sip address
   std::string          mRemoteAddress;

   // Application specific data
   void*                mTag;
    
   // Used to count number of transistions to Connected state and avoid multiple onEstablished events.
   int                  mOfferAnswerCounter;

   // List of turn prefixes related to sessioj
   std::vector<int>     mTurnPrefixList;

   // True if user agent has to send offer
   bool                 mHasToSendOffer;

   // True if user agent has to enqueue offer after ice gather finished
   bool                 mSendOfferUpdateAfterIceGather;

   // Related sip account
   PAccount             mAccount;

   // User headers for INVITE transaction
   UserHeaders          mUserHeaders;

   std::string remoteAddress() const;
   void setRemoteAddress(const std::string& address);

   void* tag();
   void setTag(void* tag);
   int sessionId();
   int increaseSdpVersion();
   int addRef();
   int release();

   // Deletes providers and media sockets
   void clearProvidersAndSockets();

   // Deletes providers
   void clearProviders();

   // Helper method to find audio provider for active sip stream
   AudioProvider* findProviderForActiveAudio();

   void processCommandList();
   void addCommand(Command* cmd);
   void enqueueOffer();
   void processQueuedOffer();
   static int generateId();
   static resip::AtomicCounter IdGenerator;
   static resip::AtomicCounter InstanceCounter;
};

typedef std::shared_ptr<Session> PSession;

/////////////////////////////////////////////////////////////////////////////////
//
// Classes that provide the mapping between Application Data and DUM 
// dialogs/dialogsets
//  										
// The DUM layer creates an AppDialog/AppDialogSet object for inbound/outbound
// SIP Request that results in Dialog creation.
//  										
/////////////////////////////////////////////////////////////////////////////////
class ResipSessionAppDialog : public resip::AppDialog
{
public:
  ResipSessionAppDialog(resip::HandleManager& ham);
  virtual ~ResipSessionAppDialog();
};

class ResipSession: public resip::AppDialogSet
{
friend class UserAgent;
friend class Account;

public:
  enum Type
  {
    Type_None,
    Type_Registration,
    Type_Subscription,
    Type_Call,
    Type_Auto
  };
  static resip::AtomicCounter InstanceCounter;


  ResipSession(resip::DialogUsageManager& dum);
  virtual ~ResipSession();
  virtual resip::AppDialog* createAppDialog(const resip::SipMessage& msg);
  virtual resip::SharedPtr<resip::UserProfile> selectUASUserProfile(const resip::SipMessage& msg);
  
  void setType(Type type);
  Type type();
  
  Session* session();
  void setSession(Session* session);
  
  UserAgent* ua();
  void setUa(UserAgent* ua);
  
  // Used for subscriptions/messages
  int sessionId();
  
  // Used for subscriptions/messages
  void* tag() const;
  void setTag(void* tag);

  // Used for subscriptions/messages
  std::string remoteAddress() const;
  void setRemoteAddress(std::string address);

  void runTerminatedEvent(Type type, int code = 0, int reason = 0);

  void setUASProfile(std::shared_ptr<resip::UserProfile> profile);

protected:
  bool mTerminated;
  UserAgent* mUserAgent;
  Type mType;
  Session* mSession;
  int mSessionId;
  std::string mRemoteAddress;
  void* mTag;
  bool mOnWatchingStartSent;
  std::shared_ptr<resip::UserProfile> mUASProfile;
};


class ResipSessionFactory : public resip::AppDialogSetFactory
{
public:
  ResipSessionFactory(UserAgent* agent);
  virtual resip::AppDialogSet* createAppDialogSet(resip::DialogUsageManager& dum, const resip::SipMessage& msg);
protected:
  UserAgent* mAgent;
}; 

#endif
