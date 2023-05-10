/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "EP_Session.h"
#include "EP_Engine.h"
#include "EP_AudioProvider.h"
#include "../media/MT_Stream.h"
#include "../media/MT_AudioStream.h"
#include "../media/MT_Dtmf.h"
#include "../helper/HL_Log.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_StreamState.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_String.h"

#define LOG_SUBSYSTEM "[Engine]"

typedef resip::SdpContents::Session::Medium Medium;
typedef resip::SdpContents::Session::MediumContainer MediumContainer;

#define DOMULTIPLEX()  mUserAgent->mConfig[CONFIG_MULTIPLEXING].asBool() ? SocketHeap::DoMultiplexing : SocketHeap::DontMultiplexing


//------------ ResipSessionAppDialog ------------
#pragma region ResipSessionAppDialog
ResipSessionAppDialog::ResipSessionAppDialog(resip::HandleManager& ham) : AppDialog(ham)
{  
}

ResipSessionAppDialog::~ResipSessionAppDialog() 
{ 
}
#pragma endregion


#pragma region ResipSession

resip::AtomicCounter ResipSession::InstanceCounter;

ResipSession::ResipSession(resip::DialogUsageManager& dum) 
    : resip::AppDialogSet(dum), mUserAgent(NULL), mType(Type_None), mSessionId(0), mSession(0)
{
    ResipSession::InstanceCounter.increment();
    mTag = NULL;
    mTerminated = false;
    mOnWatchingStartSent = false;
    mSessionId = Session::generateId();
}

ResipSession::~ResipSession()
{
    try
    {
        // Detach from user session
        if (mSession)
            mSession->mResipSession = nullptr;
        runTerminatedEvent(Type_Auto, 0, 0);
    }
    catch(...)
    {
    }

    ResipSession::InstanceCounter.decrement();
}

resip::AppDialog* ResipSession::createAppDialog(const resip::SipMessage& msg)
{
    return new ResipSessionAppDialog(static_cast<resip::HandleManager&>(mDum));
}

void ResipSession::runTerminatedEvent(Type type, int code, int reason)
{
    if (mTerminated)
        return;

    Type t = type;
    if (type == Type_Auto)
        t = mType;
    if (t == Type_None)
        t = Type_Call;

    mTerminated = true;
    if (mUserAgent)
    {
        switch (t)
        {
        case Type_Call:
            if (mSession)
                mUserAgent->onSessionTerminated(mUserAgent->getUserSession(mSessionId), code, reason);
            break;

        case Type_Registration:
            mUserAgent->onAccountStop(mUserAgent->getAccount(mSessionId), code);
            break;

        case Type_Subscription:
            if (mSession)
            {
                UserAgent::ClientObserverMap::iterator observerIter = mUserAgent->mClientObserverMap.find(mSession->sessionId());
                if (observerIter != mUserAgent->mClientObserverMap.end())
                    mUserAgent->onClientObserverStop(observerIter->second, code);
            }
            break;

        default:
            break;
        }
    }
}

std::string ResipSession::remoteAddress() const
{
    return mRemoteAddress;
}

void ResipSession::setRemoteAddress(std::string address)
{
    mRemoteAddress = address;
}

void ResipSession::setType(Type type)
{
    mType = type;
}

ResipSession::Type ResipSession::type()
{
    return mType;
}

Session* ResipSession::session()
{
    return mSession;
}

void* ResipSession::tag() const
{
    return mTag;
}

void ResipSession::setTag(void* tag)
{
    mTag = tag;
}

void ResipSession::setSession(Session* session)
{
    mSession = session;
    if (mSession)
        mSessionId = mSession->sessionId();
}

UserAgent* ResipSession::ua()
{
    return mUserAgent;
}

void ResipSession::setUa(UserAgent* ua)
{
    mUserAgent = ua;
}

int ResipSession::sessionId()
{
    return mSessionId;
}

void ResipSession::setUASProfile(std::shared_ptr<resip::UserProfile> profile)
{
    mUASProfile = profile;
}

resip::SharedPtr<resip::UserProfile> ResipSession::selectUASUserProfile(const resip::SipMessage& msg)
{
    assert(mUserAgent != nullptr);

    if (mUserAgent)
    {
        PAccount account = mUserAgent->getAccount(msg.header(resip::h_To));
        if (account)
            return account->getUserProfile();
        else
            return mUserAgent->mProfile;
    }
    return resip::SharedPtr<resip::UserProfile>();
}

#pragma endregion

#pragma region Session::Stream
Session::Stream::Stream()
    :mRtcpAttr(false), mRtcpMuxAttr(false)
{
}

Session::Stream::~Stream()
{
}

void Session::Stream::setProvider(PDataProvider provider)
{
    mProvider = provider;
}

PDataProvider Session::Stream::provider()
{
    return mProvider;
}

void Session::Stream::setSocket4(const RtpPair<PDatagramSocket>& socket)
{
    mSocket4 = socket;
}

RtpPair<PDatagramSocket>& Session::Stream::socket4()
{
    return mSocket4;
}

void Session::Stream::setSocket6(const RtpPair<PDatagramSocket>& socket)
{
    mSocket6 = socket;
}

RtpPair<PDatagramSocket>& Session::Stream::socket6()
{
    return mSocket6;
}

void Session::Stream::setIceInfo(const IceInfo& ii)
{
    mIceInfo = ii;
}

Session::IceInfo Session::Stream::iceInfo() const
{
    return mIceInfo;
}

bool Session::Stream::rtcpAttr() const
{
    return mRtcpAttr;
}

void Session::Stream::setRtcpAttr(bool value)
{
    mRtcpAttr = value;
}

bool Session::Stream::rtcpMuxAttr() const
{
    return mRtcpMuxAttr;
}

void Session::Stream::setRtcpMuxAttr(bool value)
{
    mRtcpMuxAttr = value;
}

#pragma endregion

#pragma region Session
resip::AtomicCounter Session::InstanceCounter;

Session::Session(PAccount account)
{  
    InstanceCounter.increment();
    mAccount = account;
    mSessionId = Session::generateId();
    mTag = NULL;
    mAcceptedByEngine = false;
    mAcceptedByUser = false;
    mUserAgent = NULL;
    mOriginVersion = 0;
    mSessionVersion = 0;
    mRole = Acceptor;
    mGatheredCandidates = false;
    mTerminated = false;
    mRemoteOriginVersion = (UInt64)-1;
    mResipSession = NULL;
    mRefCount = 1;
    mOfferAnswerCounter = 0;
    mHasToSendOffer = false;
    mSendOfferUpdateAfterIceGather = false;
}

Session::~Session()
{
    try
    {
        if (mResipSession)
            mResipSession->setSession(NULL);
        clearProvidersAndSockets();
    }
    catch(...)
    {}
    InstanceCounter.decrement();
}

void Session::start(const std::string& peer)
{
    ICELogInfo( << "Attempt to start session to " << peer);
    Lock l(mGuard);

    if (mResipSession)
    {
        ICELogError(<< "Session " << mSessionId << " is already started.");
        return;
    }

    // Save target address
    mRemoteAddress = UserAgent::formatSipAddress(peer);

    // Create resiprocate session
    mResipSession = new ResipSession(*mUserAgent->mDum);
    mResipSession->setSession(this);
    mResipSession->setUa(mUserAgent);

    // Do not call OnNewSession for this session
    mAcceptedByEngine = true;

    // Mark session as Initiator
    mRole = Session::Initiator;

    resip::Data addrData(peer);
    resip::NameAddr addr(addrData);

    // Save target address
    mRemotePeer = addr;

    // Start to gather ICE candidates (streams are created in addProvider() method)
    mIceStack->gatherCandidates();
}

void Session::stop()
{
    ICELogInfo(<< "Stopping session " << mSessionId);
    Lock l(mGuard);
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Session::Stream& dataStream = mStreamList[i];

        if (dataStream.provider())
        {
            // Notify provider about finished I/O
            dataStream.provider()->sessionTerminated();

            // Free socket
            SocketHeap::instance().freeSocketPair( dataStream.socket4() );
            SocketHeap::instance().freeSocketPair( dataStream.socket6() );
        }
    }

    if (mResipSession)
        mResipSession->runTerminatedEvent(ResipSession::Type_Call, 0, LocalBye);

    if (mResipSession)
        mResipSession->end();           // Stop SIP session
}

void Session::accept()
{
    ICELogInfo(<< "Attempt to accept session " << mSessionId);

    Lock locksession(mGuard);

    // If ICE candidate gathering is not finished - just mark session as accepted. It will be accepted in ICE handling code.
    mAcceptedByUser = true;

    if (mGatheredCandidates || mIceStack->state() == ice::IceNone)
    {
        ICELogInfo(<< "Candidates are gathered for session " << mSessionId << ", build&send answer.");

        // Build SDP
        resip::SdpContents sdp;
        buildSdp(sdp, Sdp_Answer);

        resip::ServerInviteSession* sis = dynamic_cast<resip::ServerInviteSession*>(mInviteHandle.get());
        if (sis)
        {
            /*if (mInviteHandle.isValid())
        mInviteHandle->setUserHeaders(mUserHeaders);*/
            sis->setUserHeaders(mUserHeaders);
            sis->provideAnswer(sdp);
            sis->accept();
        }

        // Reset mAcceptScheduled & mGatheredCandidates
        mGatheredCandidates = false;

        // Start connectivity checks
        if (mIceStack->state() > ice::IceNone)
        {
            ICELogInfo(<< "Start connectivity checks for session " << mSessionId);
            mIceStack->checkConnectivity();
        }
    }
    else
    {
        ICELogInfo(<< "ICE gathering is not finished yet for session " << mSessionId << " SDP build is deferred.");
    }
}

void Session::reject(int code)
{
    ICELogInfo( << "Attempt to reject session " << mSessionId);

    Lock l(mGuard);

    if (mInviteHandle.isValid())
    {
        mInviteHandle->reject(code);
        ICELogInfo(<< "Session " << mSessionId << " is rejected.");
    }
    else
        ICELogError(<< "Session " << mSessionId << " has not valid invite handle.");
}

AudioProvider* Session::findProviderForActiveAudio()
{
    for (unsigned streamIndex = 0; streamIndex < mStreamList.size(); streamIndex++)
    {
        PDataProvider p = mStreamList[streamIndex].provider();
        if (p)
        {
            if (p->streamName() == "audio")
            {
                AudioProvider* audio = (AudioProvider*)p.get();
                if (audio->activeStream())
                    return audio;
            }
        }
    }
    return NULL;
}

void Session::getSessionInfo(Session::InfoOptions options, VariantMap& info)
{
    Lock l(mGuard);

    // Retrieve remote sip address
    info[SessionInfo_RemoteSipAddress] = remoteAddress();
    if (mIceStack)
        info[SessionInfo_IceState] = mIceStack->state();

    // Get media stats
    MT::Statistics stat;

    // Iterate all session providers
    stat.reset();
    Stream* media = NULL;
    for (unsigned streamIndex = 0; streamIndex < mStreamList.size(); streamIndex++)
    {
        Stream& stream = mStreamList[streamIndex];
        if (stream.provider())
        {
            media = &stream;
            MT::Statistics s = stream.provider()->getStatistics();
#if defined(USE_PVQA_LIBRARY) && !defined(TARGET_SERVER)
            if (options != InfoOptions::Standard)
            {
                // This information is available AFTER audio stream is deleted
                info[SessionInfo_PvqaMos] = s.mPvqaMos;
                info[SessionInfo_PvqaReport] = s.mPvqaReport;
            }
#endif
            info[SessionInfo_NetworkMos] = static_cast<float>(s.calculateMos(4.14));
            info[SessionInfo_AudioCodec] = s.mCodecName;

            stat += s;
        }
    }

    info[SessionInfo_ReceivedTraffic] = static_cast<int>(stat.mReceived);
    info[SessionInfo_SentTraffic] = static_cast<int>(stat.mSent);
    info[SessionInfo_ReceivedRtp] = static_cast<int>(stat.mReceivedRtp);
    info[SessionInfo_ReceivedRtcp] = static_cast<int>(stat.mReceivedRtcp);
    info[SessionInfo_LostRtp] = static_cast<int>(stat.mPacketLoss);
    info[SessionInfo_DroppedRtp] = static_cast<int>(stat.mPacketDropped);
    info[SessionInfo_SentRtp] = static_cast<int>(stat.mSentRtp);
    info[SessionInfo_SentRtcp] = static_cast<int>(stat.mSentRtcp);
    if (stat.mFirstRtpTime)
        info[SessionInfo_Duration] = static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - *(stat.mFirstRtpTime)).count());
    else
        info[SessionInfo_Duration] = 0;

    if (stat.mReceivedRtp)
        info[SessionInfo_PacketLoss] = static_cast<int>((stat.mPacketLoss * 1000) / stat.mReceivedRtp);

    if (media)
        info[SessionInfo_AudioPeer] = mIceStack->remoteAddress(media->iceInfo().mStreamId, media->iceInfo().mComponentId.mRtp).toStdString();

    info[SessionInfo_Jitter] = stat.mJitter;
    if (stat.mRttDelay.is_initialized())
        info[SessionInfo_Rtt] = static_cast<float>(stat.mRttDelay * 1000);
#if defined(USE_AMR_CODEC)
    info[SessionInfo_BitrateSwitchCounter] = stat.mBitrateSwitchCounter;
#endif
    info[SessionInfo_SSRC] = stat.mSsrc;
    info[SessionInfo_RemotePeer] = stat.mRemotePeer.toStdString();
}

int Session::id() const
{
    return mSessionId;
}

PAccount Session::account()
{
    return mAccount;
}

void Session::onReceivedData(PDatagramSocket socket, InternetAddress& src, const void* receivedPtr, unsigned receivedSize)
{
    Lock l(mGuard);

    if (mTerminated)
        return;

    // Check if it STUN packet and must be processed by ICE stack
    //ICELogDebug (<< "Received UDP packet from " << src.ip() << ":" << src.port());
    ice::ByteBuffer received(receivedPtr, receivedSize);
    received.setRemoteAddress(src);

    ice::ByteBuffer plain;

    if (ice::Stack::isStun(received))
    {
        // Check if it is Data indication packet
        if (ice::Stack::isDataIndication(received, &plain))
        {
            received = plain;
        }
    }
    else // Check if packet is prefixed with TURN prefix
        if (received.size() >= 4)
        {
            bool turnPrefix = false;
            for (unsigned i=0; i<mTurnPrefixList.size() && !turnPrefix; i++)
                turnPrefix |= ice::Stack::isChannelData(received, mTurnPrefixList[i]);
            if (turnPrefix)
                received.erase(0, 4);
        }

    // Now process actual stun packet - it could be wrapped in Data Indication or Channel Data
    if (ice::Stack::isStun(received))
    {
        // Try to process incoming data by ICE stack
        int component = -1, stream = -1;
        bool processed;
        if (mIceStack->findStreamAndComponent(socket->family(), socket->localport(), &stream, &component))
        {
            ice::ByteBuffer buffer(receivedPtr, receivedSize);
            buffer.setRemoteAddress(src);
            processed = mIceStack->processIncomingData(stream, component, buffer);
        }
    }
    else
    {
        // Find provider responsible for processing
        PDataProvider provider = this->findProviderByPort(socket->family(), socket->localport());
        if (provider)
        {
            provider->processData(socket, receivedPtr, receivedSize, src);
        }
        else
        {
            // Just ignore these data
        }
    }
}

// Called when new candidate is gathered
void Session::onCandidateGathered(ice::Stack* stack, void* tag, const char* address)
{
    mUserAgent->onCandidateGathered(mUserAgent->getUserSession(mSessionId), address);
}


// Called when connectivity check is finished
void Session::onCheckFinished(ice::Stack* stack, void* tag, const char* checkDescription)
{
    mUserAgent->onCheckFinished(mUserAgent->getUserSession(mSessionId), checkDescription);
}

void Session::onGathered(ice::Stack* stack, void* tag)
{
    Lock l(mGuard);

    // This part handles situation when network was changed and new ice candidate were gathered
    if (mSendOfferUpdateAfterIceGather)
    {
        mSendOfferUpdateAfterIceGather = false;
        enqueueOffer();
        return;
    }

    // This is sending of initial offer/answer after first gather of ice candidates
    mUserAgent->onGathered(mUserAgent->getUserSession(mSessionId));

    if (mRole == Initiator)
        mUserAgent->sendOffer(this);
    else
        if (mRole == Acceptor)
        {
            // Mark session as gathered ICE candidates
            mGatheredCandidates = true;

            // if AcceptSession was already called() on session - recall it again to make real work
            if (mAcceptedByUser)
            {
                // Check if session is needed here - because session can be terminated already
                if (mResipSession && mInviteHandle.isValid())
                    accept();
            }
        }
}

void Session::onSuccess(ice::Stack* stack, void* tag)
{
    ICELogInfo(<< "ICE connectivity check succeed.");

    RtpPair<InternetAddress> t;

    for (unsigned i=0; i<this->mStreamList.size(); i++)
    {
        PDataProvider p = mStreamList[i].provider();
        if (p)
        {
            // Set new destination address
            t.mRtp = stack->remoteAddress(mStreamList[i].iceInfo().mStreamId, ICE_RTP_ID);
            // Check if there remote address for rtcp component

            t.mRtcp = stack->remoteAddress(mStreamList[i].iceInfo().mStreamId, ICE_RTCP_ID);
            if (t.mRtcp.isEmpty())
                t.mRtcp = t.mRtp;

            p->setDestinationAddress(t);

            // Notify provider about connectivity checks success
            mStreamList[i].provider()->sessionEstablished(EV_ICE);
        }
    }

    mUserAgent->onSessionEstablished(mUserAgent->getUserSession(mSessionId), EV_ICE, t);

    //time to resend updated media info over SIP
    //TODO:
}

void Session::onFailed(ice::Stack* stack, void* tag)
{
    ICELogError(<< "ICE connectivity check failed for session " << mSessionId);
    mUserAgent->onConnectivityFailed(mUserAgent->getUserSession(mSessionId));

    //if (mInviteHandle.isValid())
    //  mInviteHandle->end();
}

void Session::onNetworkChange(ice::Stack *stack, void *tag)
{
    ICELogInfo(<< "Network change detected by ICE stack for session " << mSessionId);
    mUserAgent->onNetworkChange(mUserAgent->getUserSession(mSessionId));
}

void Session::buildSdp(resip::SdpContents &sdp, SdpDirection sdpDirection)
{
    sdp.session().name() = "ICE_UA";
    sdp.session().origin().user() = "user";

    // Set versions
    sdp.version() = 0;
    sdp.session().version() = mSessionVersion;
    sdp.session().origin().getVersion() = ++mOriginVersion;

    // At least 1 stream must exists
    if (mStreamList.empty())
        return;

    // Get default ip address for front stream
    ice::NetworkAddress defaultAddr = mIceStack->defaultAddress(mStreamList.front().iceInfo().mStreamId, ICE_RTP_ID);

    // Set IP address for origin and connection
    sdp.session().origin().setAddress(resip::Data(defaultAddr.ip()), defaultAddr.family() == AF_INET ? resip::SdpContents::IP4 : resip::SdpContents::IP6);
    sdp.session().connection().setAddress(resip::Data(defaultAddr.ip()), defaultAddr.family() == AF_INET ? resip::SdpContents::IP4 : resip::SdpContents::IP6);

    // Add ICE credentials
    if (mIceStack->state() > ice::IceNone)
    {
        sdp.session().addAttribute("ice-pwd", resip::Data(mIceStack->localPassword()));
        sdp.session().addAttribute("ice-ufrag", resip::Data(mIceStack->localUfrag()));
    }

    // Iterate media streams
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Stream& stream = mStreamList[i];
        if (!stream.provider())
            continue;
        DataProvider& provider = *mStreamList[i].provider();

        // Get default stream port
        ice::NetworkAddress rtpPort = mIceStack->defaultAddress(mStreamList[i].iceInfo().mStreamId, ICE_RTP_ID),
                rtcpPort = mIceStack->defaultAddress(mStreamList[i].iceInfo().mStreamId, ICE_RTCP_ID);

        // Define media stream SDP's header
        resip::SdpContents::Session::Medium media(resip::Data(provider.streamName()), rtpPort.port(), 0, resip::Data(provider.streamProfile()));

        // Add "rtcp" attribute
        if (mUserAgent->mConfig[CONFIG_RTCP_ATTR].asBool())
        {
            if (mUserAgent->mConfig[CONFIG_MULTIPLEXING].asBool())
                rtcpPort = rtpPort;
            else
                if (rtcpPort.isEmpty())
                {
                    rtcpPort = rtpPort;
                    rtcpPort.setPort( rtpPort.port() + 1);
                }

            media.addAttribute("rtcp", resip::Data(rtcpPort.port()));
        }

        // Add "rtcp-mux" attribute
        if (mUserAgent->mConfig[CONFIG_MULTIPLEXING].asBool())
            media.addAttribute("rtcp-mux");

        // Ask provider about specific information - codecs are filled here
        provider.updateSdpOffer(media, sdpDirection);

        // Add ICE information
        std::vector<std::string> candidates;
        if (stream.iceInfo().mStreamId != -1)
        {
            IceInfo ii = stream.iceInfo();
            mIceStack->fillCandidateList(ii.mStreamId, ii.mComponentId.mRtp, candidates);

            if (mIceStack->hasComponent(ii.mStreamId, ii.mComponentId.mRtcp))
                mIceStack->fillCandidateList(ii.mStreamId, ii.mComponentId.mRtcp, candidates);

            for (unsigned c=0; c<candidates.size(); c++)
                media.addAttribute("candidate", candidates[c].c_str());
        }

        // Add stream to session
        sdp.session().addMedium(media);
    }
}

PDataProvider Session::findProviderByPort(int family, unsigned short port)
{
    for (unsigned i = 0; i < mStreamList.size(); i++)
    {
        Stream& s = mStreamList[i];

        if ((s.socket4().mRtp->localport() == port || s.socket4().mRtcp->localport() == port) && family == AF_INET)
            return s.provider();
        if ((s.socket6().mRtp->localport() == port || s.socket6().mRtcp->localport() == port) && family == AF_INET6)
            return s.provider();
    }

    return PDataProvider();
}

void Session::addProvider(PDataProvider provider)
{
    // Ignore NULL providers
    if (!provider)
        return;

    // Avoid duplicating providers
    for (unsigned i=0; i<mStreamList.size(); i++)
        if (mStreamList[i].provider() == provider)
            return;

    // Find first non-filled record or create new one
    std::vector<Stream>::iterator streamIter;

    for (streamIter = mStreamList.begin(); streamIter != mStreamList.end(); ++streamIter)
    {
        if (!streamIter->provider() && (streamIter->socket4().mRtp->isValid() || streamIter->socket6().mRtp->isValid()))
        {
            streamIter->setProvider( provider );
            provider->setSocket(streamIter->socket4(), streamIter->socket6());
            return;
        }
    }

    Stream s;
    s.setProvider( provider );

    // Allocate socket for provider
    s.setSocket4( SocketHeap::instance().allocSocketPair(AF_INET, this, DOMULTIPLEX()) );
    s.setSocket6( SocketHeap::instance().allocSocketPair(AF_INET6, this, DOMULTIPLEX()) );
    s.provider()->setSocket(s.socket4(), s.socket6());

    // Create ICE stream/component
    IceInfo ii;
    ii.mStreamId = mIceStack->addStream();
    ii.mPort4 = s.socket4().mRtp->localport();
    ii.mPort6 = s.socket6().mRtp->localport();

    ii.mComponentId.mRtp = mIceStack->addComponent(ii.mStreamId, NULL, s.socket4().mRtp->localport(),
                                                   s.socket6().mRtp->localport());
    if (!mUserAgent->mConfig[CONFIG_MULTIPLEXING].asBool())
        ii.mComponentId.mRtcp = mIceStack->addComponent(ii.mStreamId, NULL, s.socket4().mRtcp->localport(), s.socket6().mRtcp->localport());

    s.setIceInfo(ii);

    mStreamList.push_back(s);
}

PDataProvider Session::providerAt(int index)
{
    if (mStreamList.size() > unsigned(index))
        return mStreamList[index].provider();
    return PDataProvider();
}

int Session::getProviderCount()
{
    return mStreamList.size();
}

int Session::sessionId()
{
    return mSessionId;
}

resip::AtomicCounter Session::IdGenerator;
int Session::generateId()
{
    return (int)IdGenerator.increment();
}

std::string Session::remoteAddress() const
{
    return mRemoteAddress;
}

void Session::setRemoteAddress(const std::string& address)
{
    mRemoteAddress = address;
}

void Session::setUserHeaders(const UserHeaders& headers)
{
    mUserHeaders = headers;
}

void* Session::tag()
{
    return mTag;
}

void Session::setTag(void* tag)
{
    mTag = tag;
}

void Session::pause()
{
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Stream& s = mStreamList[i];
        if (s.provider())
            s.provider()->pause();
    }
    enqueueOffer();
}

void Session::resume()
{
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Stream& s = mStreamList[i];
        if (s.provider())
            s.provider()->resume();
    }
    enqueueOffer();
}

void Session::refreshMediaPath()
{
    // Recreate media sockets
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Stream& s= mStreamList[i];
        PDataProvider p = s.provider();
        if (!p)
            continue;

        // Close old socket
        SocketHeap::instance().freeSocketPair(p->socket(AF_INET));

        // Bring new socket to provider and stream
        RtpPair<PDatagramSocket> s4 = SocketHeap::instance().allocSocketPair(AF_INET, this, DOMULTIPLEX() ),
                s6 = SocketHeap::instance().allocSocketPair(AF_INET, this, DOMULTIPLEX());

        p->setSocket(s4, s6);
        s.setSocket4(s4);
        s.setSocket6(s6);
    }

    // Recreate new ufrag/pwd and gather ice candidates
    mIceStack->refreshPwdUfrag();
    mIceStack->gatherCandidates();

    // New offer will be enqueued after ice gather finished
    mSendOfferUpdateAfterIceGather = true;
    mHasToSendOffer = false;
}

// Received offer with new SDP
int Session::processSdp(UInt64 version, bool iceAvailable, std::string icePwd, std::string iceUfrag,
                        std::string remoteIp, const resip::SdpContents::Session::MediumContainer& media)
{
    bool iceRestart = false;

    int mediaCompatible = 0;
    MediumContainer::const_iterator mediaIter;
    unsigned streamIndex = 0;
    for (mediaIter = media.begin(); mediaIter != media.end(); ++mediaIter, ++streamIndex)
    {
        // Get reference to SDP description of remote stream
        const resip::SdpContents::Session::Medium& remoteStream = *mediaIter;

        // Get reference to local stream description
        if (streamIndex >= mStreamList.size())
            mStreamList.push_back(Session::Stream());
        Session::Stream& stream = mStreamList[streamIndex];

        // Ask about provider if needed
        if (!stream.provider())
            stream.setProvider( mUserAgent->onProviderNeeded(remoteStream.name().c_str()) );

        // Check the stream validity
        if (!stream.provider())
            continue;
        if (!stream.provider()->processSdpOffer(remoteStream, Sdp_Offer))
            continue;

        // See for rtcp & rtcp-mux attribute
        stream.setRtcpAttr( remoteStream.exists("rtcp") );
        stream.setRtcpMuxAttr( remoteStream.exists("rtcp-mux") );

        // Set destination address
        if (!remoteStream.getConnections().empty())
            remoteIp = remoteStream.getConnections().front().getAddress().c_str();

        RtpPair<InternetAddress> targetAddr;
        targetAddr.mRtp.setIp(remoteIp);
        targetAddr.mRtp.setPort(remoteStream.port());

        targetAddr.mRtcp.setIp(remoteIp);
        if (stream.rtcpMuxAttr())
            targetAddr.mRtcp.setPort( remoteStream.port() );
        else
            if (stream.rtcpAttr())
                targetAddr.mRtcp.setPort( strx::toInt(remoteStream.getValues("rtcp").front().c_str(), remoteStream.port() + 1 ) );
            else
                targetAddr.mRtcp.setPort( remoteStream.port() + 1);

        stream.provider()->setDestinationAddress(targetAddr);

        // Media is compatible; increase the counter and prepare ice stream/component
        mediaCompatible++;

        // Get default remote port number
        unsigned short remotePort = remoteStream.port();

        // Create media socket if needed
        if (!stream.socket4().mRtp)
        {
            try
            {
                stream.setSocket4(SocketHeap::instance().allocSocketPair(AF_INET, this, DOMULTIPLEX()));
                stream.setSocket6(SocketHeap::instance().allocSocketPair(AF_INET6, this, DOMULTIPLEX()));
            }
            catch(...)
            {
                ICELogError( << "Cannot create media socket.");
                return 503;
            }

            // Update provider with socket references
            stream.provider()->setSocket(stream.socket4(), stream.socket6());
        }


        const std::list<resip::Data>& iceUfragAttr = remoteStream.getValues("ice-ufrag");
        if (iceUfragAttr.size())
            iceUfrag = iceUfragAttr.front().c_str();

        const std::list<resip::Data>& icePwdAttr = remoteStream.getValues("ice-pwd");
        if (icePwdAttr.size())
            icePwd = icePwdAttr.front().c_str();

        // Create ICE stream; it will be created even if ice is not requested.
        // The cause is we need to hold default address and possible STUN requests result
        if (stream.iceInfo().mStreamId == -1)
        {
            IceInfo ii;
            ii.mStreamId = mIceStack->addStream();
            ii.mPort4 = stream.socket4().mRtp->localport();
            ii.mPort6 = stream.socket6().mRtp->localport();
            ii.mComponentId.mRtp = mIceStack->addComponent(ii.mStreamId, NULL, ii.mPort4, ii.mPort6);

            // See what remote peer offers - offer only single ice component if it relies on multiplexing
            if (!targetAddr.multiplexed() && !mUserAgent->mConfig[CONFIG_MULTIPLEXING].asBool())
                ii.mComponentId.mRtcp = mIceStack->addComponent(ii.mStreamId, NULL, stream.socket4().mRtcp->localport(), stream.socket6().mRtcp->localport());
            stream.setIceInfo(ii);
        }

        if (iceAvailable)
        {
            if (mIceStack->remotePassword(stream.iceInfo().mStreamId) != icePwd || mIceStack->remoteUfrag(stream.iceInfo().mStreamId) != iceUfrag)
            {
                iceRestart = true;
                mIceStack->setRemotePassword(icePwd, stream.iceInfo().mStreamId);
                mIceStack->setRemoteUfrag(iceUfrag, stream.iceInfo().mStreamId);
            }
        }

        // Get remote ICE candidates vector
        const std::list<resip::Data> candidateList = remoteStream.getValues("candidate");

        // Repackage information about remote candidates
        std::vector<std::string> candidateVector;
        std::list<resip::Data>::const_iterator cit = candidateList.begin();

        for (; cit != candidateList.end(); ++cit)
            candidateVector.push_back(cit->c_str());

        if (candidateVector.empty())
            iceAvailable = false;

        // Ask ICE stack to process this information. This call will remove also second component if it is not defined in remote sdp.
        if (iceAvailable)
            iceAvailable = mIceStack->processSdpOffer(stream.iceInfo().mStreamId, candidateVector, remoteIp, remotePort, mUserAgent->mConfig[CONFIG_DEFERRELAYED].asBool());
    }

    // See if there are compatible media streams
    if (!mediaCompatible)
        return 488; // Not acceptable media

    // Start ICE check connectivity if required
    if (iceRestart && iceAvailable)
    {
        if (mIceStack->state() > ice::IceGathering)
            mIceStack->checkConnectivity();
        else
            mIceStack->gatherCandidates();
    }

    return 200;
}

int Session::increaseSdpVersion()
{
    return ++mOriginVersion;
}

void Session::setUserAgent(UserAgent* agent)
{
    mUserAgent = agent;
}

UserAgent* Session::userAgent()
{
    return mUserAgent;
}

int Session::addRef()
{
    return ++mRefCount;
}

int Session::release()
{
    mRefCount--;
    return mRefCount;
}

void Session::clearProvidersAndSockets()
{
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Session::Stream& ds = mStreamList[i];

        if (ds.provider())
        {
            ds.provider()->sessionDeleted();
            SocketHeap::instance().freeSocketPair( ds.socket4() );
            SocketHeap::instance().freeSocketPair( ds.socket6() );
        }
    }
}

void Session::clearProviders()
{
    for (unsigned i=0; i<mStreamList.size(); i++)
    {
        Session::Stream& ds = mStreamList[i];

        if (ds.provider())
            ds.provider()->sessionTerminated();
    }
}

#pragma endregion


void Session::enqueueOffer()
{
    mHasToSendOffer = true;
    processQueuedOffer();
}

void Session::processQueuedOffer()
{
    if (!mHasToSendOffer)
        return;

    if (mInviteHandle.isValid())
    {
        if (mInviteHandle->canProvideOffer())
        {
            mUserAgent->sendOffer(this);
            mHasToSendOffer = false;
        }
    }
}

//-------------- ResipSessionFactory ---------
#pragma region ResipSessionFactory

ResipSessionFactory::ResipSessionFactory(UserAgent* agent)
    :mAgent(agent)
{}

resip::AppDialogSet* ResipSessionFactory::createAppDialogSet(resip::DialogUsageManager& dum, const resip::SipMessage& msg)
{  
    ResipSession* s = new ResipSession(dum);
    s->setUa( mAgent );
    return s;
}


#pragma endregion
