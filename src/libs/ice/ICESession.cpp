/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEPlatform.h"
#include "ICESession.h"
#include "ICENetworkHelper.h"
#include "ICEAction.h"
#include "ICEBinding.h"
#include "ICERelaying.h"
#include "ICEAddress.h"
#include "ICEStream.h"
#include "ICELog.h"
#include "ICEStunAttributes.h"

#include <algorithm>
#ifdef TARGET_WIN
#include <iphlpapi.h>
#endif

#include <assert.h>
#include <set>
#include <map>
#include <exception>
#include <stdexcept>

using namespace ice;


#define LOG_SUBSYSTEM "ICE"


Session::Session()
{
  //No activity yet
  mState = None; 
  
  //start interval for STUN messages
  mStartInterval = 20; 

  mLocalPortNumber = 0;
  refreshPwdUfrag();  
  
  //default role is Controlled
  mRole = RoleControlled;
  
  mTieBreaker = "        ";
  for (size_t i=0; i<8; i++)
    mTieBreaker[i] = rand() & 0xFF;

  mFoundationGenerator = 0xFFFFFFFF;
  mMustRestart = false;
}

Session::~Session()
{
}

void Session::setup(StackConfig& config)
{
  if (!config.mUseSTUN && !config.mUseTURN)
    throw std::logic_error("ICE is configured to not use STUN and not use TURN.");
  if (config.mUseSTUN && config.mUseTURN)
    throw std::logic_error("ICE is configured to use both STUN and TURN.");

  // Save the configuration
  mConfig = config;

  std::map<int, ICEStreamPtr>::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->setConfig(config);
}

int Session::addStream()
{
  Lock lock(mGuard);

  ICEStreamPtr s(new Stream());
  s->setAgentRole(mRole);
  s->setConfig(mConfig);
  s->mTieBreaker = mTieBreaker;
  s->setLocalPwd(mLocalPwd);
  s->setLocalUfrag(mLocalUfrag);
  s->mId = mStreamMap.size();
  mStreamMap[s->mId] = s;

  ICELogInfo(<< "New stream " << s->mId << " is add.");

  return s->mId;
}

int Session::addComponent(int streamID, void* tag, unsigned short port4, unsigned short port6)
{
  Lock lock(mGuard);

  if (mStreamMap.find(streamID) == mStreamMap.end())
  {
    ICELogError(<< "Cannot find stream " << streamID << " to add new component.");
    return -1;
  }

  // Get reference to stream
  Stream& stream = *mStreamMap[streamID];
  
  // Assign new ID to component
  int componentID = stream.addComponent(tag, port4, port6);

  ICELogInfo( << "New component " << componentID << " is add to stream " << streamID << ".");

  return componentID;
}

void Session::removeStream(int streamID)
{
  Lock lock(mGuard);

  std::map<int, ICEStreamPtr>::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    mStreamMap.erase(streamIter);
}

bool Session::findStreamAndComponent(int family, unsigned short port, int* stream, int* component)
{
  Lock lock(mGuard);

  // Iterate streams
  ICEStreamMap::iterator sit;
  for (sit = mStreamMap.begin(); sit != mStreamMap.end(); ++sit)
  {
    if (sit->second->hasPortNumber(family, port, component))
    {
      *stream = sit->first;
      return true;
    }
  }

  return false;
}

bool Session::hasStream(int streamId)
{
	Lock l(mGuard);
	return mStreamMap.find(streamId) != mStreamMap.end();
}

bool Session::hasComponent(int streamId, int componentId)
{
  Lock l(mGuard);
  ICEStreamMap::const_iterator streamIter = mStreamMap.find(streamId);
  if (streamIter == mStreamMap.end())
    return false;
  if (!streamIter->second)
    return false;

  return streamIter->second->mComponentMap.find(componentId) != streamIter->second->mComponentMap.end();
}

void Session::setComponentPort(int streamId, int componentId, unsigned short port4, unsigned short port6)
{
  Lock l(mGuard);
  if (hasComponent(streamId, componentId))
  {
	  mStreamMap[streamId]->mComponentMap[componentId].mPort4 = port4;
	  mStreamMap[streamId]->mComponentMap[componentId].mPort6 = port6;
  }
}


void Session::gatherCandidates()
{
  //protect instance
  Lock lock(mGuard);

  //parse the STUN/TURN server IP address
  //mConfig.mSTUNServerAddr.GetSockaddr((sockaddr&)mSTUNServerAddr);
  //mConfig.mTURNServerAddr.GetSockaddr((sockaddr&)mTURNServerAddr);

  //as RFC says...
  //if (mConfig.mUseTURN)
  //  mSTUNServerAddr = mTURNServerAddr;

  // Define the list of used IP interfaces
  std::vector<NetworkAddress> hostip;
  
  // Get list of IP interfaces
  // std::vector<NetworkAddress>& ipList = NetworkHelper::Instance().GetInterfaceList();

  // Iterate all streams and instructs them to gather candidates
  std::map<int, ICEStreamPtr>::iterator streamIter;
  int finished = 0;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
  {
    streamIter->second->gatherCandidates();
    if (streamIter->second->mState > CandidateGathering)
      finished++;
  }
  
  // Set session state as "candidate gathering" or "creating sdp" - depending on stream state
  if ((size_t)finished == mStreamMap.size())
    mState = CreatingSDP;
  else
    mState = CandidateGathering;
}



bool Session::processData(ByteBuffer& buffer, int streamID, int component)
{
  Lock lock(mGuard);

  // Attempt to parse
  StunMessage msg;
  if (!msg.parsePacket(buffer))
    return false;
  
  ICELogDebug( << "Received STUN packet from " << buffer.remoteAddress().toStdString().c_str() << ". Data: " << buffer.hexstring());
  
  // Old&new states
  int oldstate = None, newstate = None;

  // Find stream responsible for this buffer
  if (mStreamMap.find(streamID) == mStreamMap.end())
    return false;
  
  // Get pointer to stream
  ICEStreamPtr streamPtr = mStreamMap[streamID];
  if (!streamPtr)
    return false;
  
  Stream& stream = *streamPtr;
  oldstate = stream.mState;
  /*bool result = */stream.processData(msg, buffer, component);
  newstate = stream.mState;

  if (newstate != oldstate)
  {
    // State is changed. Maybe full session state is changed too?
    int failedcount = 0, successcount = 0;
    
    switch (newstate)
    {
    case CreatingSDP:

      // Check if all streams gathered candidates or failed
      for (ICEStreamMap::iterator si=mStreamMap.begin(); si != mStreamMap.end(); ++si)
      {
        if (!si->second)
          continue;
        
        Stream& stream = *si->second;
        if (stream.mState == CreatingSDP)
          successcount++;
        else
        if (stream.mState == Failed)
          failedcount++;
      }

      if (failedcount == (int)mStreamMap.size())
        mState = Failed;
      else
        mState = CreatingSDP;

      break;

    case Success:
    case Failed:
      // Check if all streams a success or failed
      for (ICEStreamMap::iterator si=mStreamMap.begin(); si != mStreamMap.end(); ++si)
      {
        if (!si->second)
          continue;
        
        Stream& stream = *si->second;
        if (stream.mState == Success)
          successcount++;
        else
        if (stream.mState == Failed)
          failedcount++;
      }

      if (failedcount == (int)mStreamMap.size())
        mState = Failed;
      else
        mState = Success;

      break;

    }
  }
  return true;
}

PByteBuffer Session::getDataToSend(bool& response, int& stream, int& component, void*&tag)
{
  // Protect instance
  Lock lock(mGuard);
  
  PByteBuffer result;

  // Iterate streams to update their states (maybe timeout occured since last call of getDataToSend())
  ICEStreamMap::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    if (streamIter->second)
      streamIter->second->isTimeout();
  
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
  {
    if (streamIter->second)
      result = streamIter->second->getDataToSend(response, component, tag);
    if (result)
    {
      stream = streamIter->first;
      return result;
    }
  }
  return result;
}

bool Session::active()
{
  //protect instance
  Lock lock(mGuard);
  
  return (mState != None);
}

void Session::createSdp(std::vector<std::string>& common)
{
  common.push_back("a=ice-full");
  common.push_back("a=ice-pwd:" + localPwd());
  common.push_back("a=ice-ufrag:" + localUfrag());
}

void Session::createSdp(int streamID, std::vector<std::string>& defaultIP, 
                                 std::vector<unsigned short>& defaultPort, std::vector<std::string>& candidateList)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    streamIter->second->createOfferSdp(defaultIP, defaultPort, candidateList);
}

  
bool Session::findCandidate(std::vector<Candidate>&cv, Candidate::Type _type, 
      const std::string& baseIP, int componentID, Candidate& result)
{
  for (unsigned int i=0; i<cv.size(); i++)
  {
    Candidate& c = cv[i];
    if (c.mType == _type && c.mComponentId == componentID && c.mLocalAddr.ip()/*mBase*/ == baseIP)
    {
      result = c;
      return true;
    }
  }

  return false;
}

RunningState Session::state()
{
  return mState;
}

std::string  Session::createUfrag()
{
  std::string result;
  result.resize(4);

  for (size_t i=0; i<4; i++)
  {
    int r = rand();
    char c = 'a' + int((float)r / ((float)RAND_MAX / 26.0));
    
    result[i] = c;
  }

  return result;
}

std::string  Session::createPassword()
{
  std::string result;
  result.resize(22);

  for (size_t i=0; i<22; i++)
  {
    int r = rand();
    char c = 'a' + int((float)r / ((float)RAND_MAX / 26.0));
    
    result[i] = c;
  }

  return result;
}

std::string Session::localUfrag()
{
  return mLocalUfrag;
}

std::string Session::localPwd()
{
  return mLocalPwd;
}

void Session::setRemotePassword(const std::string& pwd, int streamId)
{
  mMustRestart |= pwd != mRemotePwd;
  mRemotePwd = pwd;
  for (ICEStreamMap::iterator streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); streamIter++)
  {
    ICEStreamPtr s = streamIter->second;
    if (s)
    {
      if (streamId == -1 || streamIter->first == streamId)
        s->setRemotePwd(pwd);
    }
  }

}

std::string Session::remotePassword(int streamId) const
{
  if (streamId == -1)
    return mRemotePwd;
  ICEStreamMap::const_iterator streamIter = mStreamMap.find(streamId);
  if (streamIter == mStreamMap.end())
    return std::string();
  if (!streamIter->second)
    return std::string();
  
  return streamIter->second->mRemotePwd;
}

void Session::setRemoteUfrag(const std::string& ufrag, int streamId)
{
  mMustRestart |= ufrag != mRemoteUfrag;
  mRemoteUfrag = ufrag;

  // Set it to streams
  for (ICEStreamMap::iterator streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); streamIter++)
  {
    ICEStreamPtr s = streamIter->second;
    if (s)
    {
      if (streamId == -1 || streamIter->first == streamId)
        s->setRemoteUfrag(ufrag);
    }
  }
}

std::string Session::remoteUfrag(int streamId) const
{
  if (streamId == -1)
    return mRemoteUfrag;
  ICEStreamMap::const_iterator streamIter = mStreamMap.find(streamId);
  if (streamIter == mStreamMap.end())
    return std::string();
  if (!streamIter->second)
    return std::string();

  return streamIter->second->mRemoteUfrag;
}

bool Session::processSdpOffer(int streamIndex, std::vector<std::string>& candidateList,
                          const std::string& defaultIP, unsigned short defaultPort, bool deleteRelayed)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamIndex);
  if (streamIter != mStreamMap.end())
    return streamIter->second->processSdpOffer(candidateList, defaultIP, defaultPort, deleteRelayed);
  
  return false;
}

NetworkAddress Session::getRemoteRelayedCandidate(int stream, int component)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(stream);
  if (streamIter != mStreamMap.end())
    return streamIter->second->remoteRelayedAddress(component);
  
  return NetworkAddress();
}

NetworkAddress Session::getRemoteReflexiveCandidate(int stream, int component)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(stream);
  if (streamIter != mStreamMap.end())
    return streamIter->second->remoteReflexiveAddress(component);
  
  return NetworkAddress();
}

NetworkAddress Session::defaultAddress(int streamID, int componentID)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    return streamIter->second->defaultAddress(componentID);
  return NetworkAddress();
}

void Session::fillCandidateList(int streamID, int componentID, std::vector<std::string>& candidateList)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    streamIter->second->candidateList(componentID, candidateList);
}

void Session::checkConnectivity()
{
  ICELogInfo( << "Starting connectivity checks.");

  // Check current session state to ensure is did not run connectivity checks already
  if (mState == ConnCheck || mState == Success)
    return;
  
  // Make current state "connection checking now"
  mState = ConnCheck;
  
  std::map<int, ICEStreamPtr>::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); streamIter++)
  {
    streamIter->second->setRemotePwd(mRemotePwd);
    streamIter->second->setRemoteUfrag(mRemoteUfrag);
    streamIter->second->startChecks();
  }

  // Set session state to "connection checks"
  mState = ConnCheck;

  /*
   Checks are generated only by full implementations.  Lite
   implementations MUST skip the steps described in this section.

   An agent performs ordinary checks and triggered checks.  The
   generation of both checks is governed by a timer which fires
   periodically for each media stream.  The agent maintains a FIFO
   queue, called the triggered check queue, which contains candidate
   pairs for which checks are to be sent at the next available

   opportunity.  When the timer fires, the agent removes the top pair
   from triggered check queue, performs a connectivity check on that
   pair, and sets the state of the candidate pair to In-Progress.  If
   there are no pairs in the triggered check queue, an ordinary check is
   sent.

   Once the agent has computed the check lists as described in
   Section 5.7, it sets a timer for each active check list.  The timer
   fires every Ta*N seconds, where N is the number of active check lists
   (initially, there is only one active check list).  Implementations
   MAY set the timer to fire less frequently than this.  Implementations
   SHOULD take care to spread out these timers so that they do not fire
   at the same time for each media stream.  Ta and the retransmit timer
   RTO are computed as described in Section 16.  Multiplying by N allows
   this aggregate check throughput to be split between all active check
   lists.  The first timer fires immediately, so that the agent performs
   a connectivity check the moment the offer/answer exchange has been
   done, followed by the next check Ta seconds later (since there is
   only one active check list).

    When the timer fires, and there is no triggered check to be sent, the
   agent MUST choose an ordinary check as follows:

   o  Find the highest priority pair in that check list that is in the
      Waiting state.

   o  If there is such a pair:

      *  Send a STUN check from the local candidate of that pair to the
         remote candidate of that pair.  The procedures for forming the
         STUN request for this purpose are described in Section 7.1.1.

      *  Set the state of the candidate pair to In-Progress.

   o  If there is no such pair:

      *  Find the highest priority pair in that check list that is in
         the Frozen state.

      *  If there is such a pair:

         +  Unfreeze the pair.

         +  Perform a check for that pair, causing its state to
            transition to In-Progress.

      *  If there is no such pair:

         +  Terminate the timer for that check list.

   To compute the message integrity for the check, the agent uses the
   remote username fragment and password learned from the SDP from its
   peer.  The local username fragment is known directly by the agent for
   its own candidate.
  */

}

void Session::setRole(AgentRole role)
{
  mRole = role;

  std::map<int, ICEStreamPtr>::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->setAgentRole(role);
}

AgentRole Session::role()
{
  return mRole;
}


void Session::clear()
{
  refreshPwdUfrag();
  mState = None;
  std::map<int, ICEStreamPtr>::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->clear();
  mTurnPrefixMap.clear();
}

void Session::clearForRestart(bool localNetworkChanged)
{
  refreshPwdUfrag();
  mState = ice::None;
  ICEStreamMap::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->clearForRestart(localNetworkChanged);
}

void Session::stopChecks()
{
  ICELogInfo(<< "Stop connectivity checks");

  ICEStreamMap::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->stopChecks();
  
  if (mState < Failed)
    mState = Failed;
}

void Session::cancelAllocations()
{
  for (auto stream: mStreamMap)
    stream.second->cancelAllocations();
}

bool Session::finished()
{
  return (mState == Failed || mState == Success);
}

NetworkAddress Session::remoteAddress(int streamID, int componentID)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    return streamIter->second->remoteAddress(componentID);
  
  return NetworkAddress();
}

NetworkAddress Session::localAddress(int streamID, int componentID)
{
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    return streamIter->second->localAddress(componentID);
  
  return NetworkAddress();
}

NetworkAddress Session::reflexiveAddress(int streamID, int componentID)
{
  Lock l(mGuard);
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    return streamIter->second->reflexiveAddress(componentID);
  
  return NetworkAddress();
}
    
NetworkAddress Session::relayedAddress(int streamID, int componentID)
{
  Lock l(mGuard);
  ICEStreamMap::iterator streamIter = mStreamMap.find(streamID);
  if (streamIter != mStreamMap.end())
    return streamIter->second->relayedAddress(componentID);
  
  return NetworkAddress();
}

bool Session::hasTurnPrefix(TurnPrefix prefix)
{
  Lock l(mGuard);
  TurnPrefixMap::iterator mit = mTurnPrefixMap.find(prefix);
  return mit != mTurnPrefixMap.end();
}

void Session::chooseDefaults()
{
  Lock l(mGuard);

  ICEStreamMap::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->Handle_CD();
}

void Session::dump(std::ostream& output)
{
  Lock l(mGuard);

  ICEStreamMap::iterator streamIter;
  for (streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
    streamIter->second->dump(output);
}

bool Session::findConcludePair(int stream, Candidate& local, Candidate& remote)
{
  Lock l(mGuard);

  ICEStreamMap::iterator streamIter = mStreamMap.find(stream);
  if (streamIter != mStreamMap.end())
    return streamIter->second->findConcludePair(local, remote);
  return false;
}

bool Session::mustRestart() const
{
  return mMustRestart;
}

void Session::refreshPwdUfrag()
{
  mLocalPwd = Session::createPassword();
  mLocalUfrag = Session::createUfrag();

  // Update all streams
  for (ICEStreamMap::iterator streamIter = mStreamMap.begin(); streamIter != mStreamMap.end(); ++streamIter)
  {
    if (streamIter->second)
    {
      streamIter->second->setLocalPwd(mLocalPwd);
      streamIter->second->setLocalUfrag(mLocalUfrag);
    }
  }
}

void Session::freeAllocation(int stream, int component, DeleteAllocationCallback* cb)
{
  Lock l(mGuard);
  
  ICEStreamMap::iterator streamIter = mStreamMap.find(stream);
  if (streamIter != mStreamMap.end())
    streamIter->second->freeAllocation(component, cb);
}

bool Session::hasAllocations()
{
  int allocated = 0;
  Lock l(mGuard);
  
  ICEStreamMap::iterator streamIter = mStreamMap.begin();
  for (;streamIter != mStreamMap.end(); ++streamIter)
    if (streamIter->second)
      allocated += streamIter->second->mTurnAllocated;
  
  return allocated > 0;
}

bool Session::isDataIndication(ByteBuffer& source, ByteBuffer* plain)
{
  StunMessage msg;
  if (!msg.parsePacket(source))
    return false;
  
  if (msg.hasAttribute(StunAttribute::Data) && msg.hasAttribute(StunAttribute::XorPeerAddress))
  {
    // It is Data indication packet
    DataAttribute& d = dynamic_cast<DataAttribute&>(msg.attribute(StunAttribute::Data));
    XorPeerAddress& xpa = dynamic_cast<XorPeerAddress&>(msg.attribute(StunAttribute::XorPeerAddress));
    
    if (plain)
    {
      *plain = d.data();
      NetworkAddress remoteAddress = xpa.address();
      remoteAddress.setRelayed(true);
      plain->setRemoteAddress(remoteAddress);
      plain->setRelayed(true);
    }
    return true;
  }
  else
    return false;
}

bool Session::isStun(const ByteBuffer& source)
{
  if (source.size() < 8)
    return false;

  const unsigned* magic = (const unsigned*)source.data();
  return (ntohl(magic[1]) == 0x2112A442);
}


int Session::errorCode()
{
  Lock l(mGuard);
  ICEStreamMap::const_iterator streamIter = mStreamMap.begin();
  for (;streamIter != mStreamMap.end(); ++streamIter)
    if (streamIter->second->mState == /*RunningState::*/Failed)
      return streamIter->second->mErrorCode;
  return 0;
}

TurnPrefix Session::bindChannel(int stream, int component, const NetworkAddress& target, ChannelBoundCallback* cb)
{
  Lock l(mGuard);
  ICELogInfo(<< "Bind channel " << stream << "/" << component << " to " << target.toStdString());

  ICEStreamMap::iterator streamIter = mStreamMap.find(stream);
  if (streamIter != mStreamMap.end())
    return streamIter->second->bindChannel(target, component, cb);
  
  return 0; // Channel numbers are in range [0x4000...0x7FFF]
}

bool Session::isChannelBindingFailed(int stream, int component, TurnPrefix prefix)
{
  Lock l(mGuard);
  ICEStreamMap::iterator streamIter = mStreamMap.begin();
  if (streamIter != mStreamMap.end())
    if (streamIter->second)
      return streamIter->second->isChannelBindingFailed(component, prefix);

  return false;
}

void Session::installPermissions(int stream, int component, const NetworkAddress &address, InstallPermissionsCallback* cb)
{
  Lock l(mGuard);
  ICELogInfo(<< "Install permissions " << stream << "/" << component << " for " << address.toStdString());

  ICEStreamMap::iterator streamIter = mStreamMap.begin();
  if (streamIter != mStreamMap.end())
    streamIter->second->installPermissions(component, address, cb);
}

bool Session::isRtp(ByteBuffer& source)
{
  if (!source.size())
    return false;
  unsigned char b = *(const unsigned char*)source.data();
  return (b & 0xC0) == 0x80;
}

bool Session::isChannelData(ByteBuffer& source, TurnPrefix prefix)
{
  if (source.size() < 4)
    return false;

  if (!prefix)
  {
    unsigned char b = *(const unsigned char*)source.data();
    return (b & 0xC0) == 0x40;
  }
  else
  {
    unsigned short pp = ntohs(*(const unsigned short*)source.data());
    return pp == prefix;
  }
}

Stream::CandidateVector* Session::remoteCandidates(int stream)
{
  ICEStreamMap::iterator iter = mStreamMap.find(stream);
  if (iter != mStreamMap.end())
    return &iter->second->mRemoteCandidate;
  else
    return NULL;
}

NetworkAddress Session::activeStunServer(int stream) const
{
  ICEStreamMap::const_iterator iter = mStreamMap.find(stream);
  if (iter != mStreamMap.end())
    return iter->second->mConfig.mServerAddr4;
  else
    return NetworkAddress();
}
