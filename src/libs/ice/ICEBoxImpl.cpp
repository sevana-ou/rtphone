/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEBoxImpl.h"
#include "ICELog.h"

#include <assert.h>

#define LOG_SUBSYSTEM "ICE"

using namespace ice;

StackImpl::StackImpl(const ServerConfig& config)
:mConfig(config), mEventHandler(NULL), mEventTag(NULL), mTimeout(false), mActionTimestamp(0)
{
  setup(config);
}

StackImpl::~StackImpl()
{
}

void StackImpl::setEventHandler(StageHandler* handler, void* tag)
{
  mEventHandler = handler;
  mEventTag = tag;
}

int StackImpl::addStream()
{
  return mSession.addStream();
}

int StackImpl::addComponent(int streamID, void* tag, unsigned short port4, unsigned short port6)
{
  return mSession.addComponent(streamID, tag, port4, port6);
}

void StackImpl::removeStream(int streamID)
{
  mSession.removeStream(streamID);
}

bool StackImpl::findStreamAndComponent(int family, unsigned short port, int* stream, int* component)
{
  return mSession.findStreamAndComponent(family, port, stream, component);
}

bool StackImpl::hasStream(int streamId)
{
	return mSession.hasStream(streamId);
}

bool StackImpl::hasComponent(int streamId, int componentId)
{
  return mSession.hasComponent(streamId, componentId);
}

void StackImpl::setComponentPort(int streamId, int componentId, unsigned short port4, unsigned short port6)
{
	mSession.setComponentPort(streamId, componentId, port4, port6);
}


void StackImpl::setRole(AgentRole role)
{
  mSession.setRole(role);
}

AgentRole StackImpl::role()
{
  return mSession.role();
}

bool StackImpl::processIncomingData(int stream, int component, ByteBuffer& incomingData)
{
#if defined(ICE_EMULATE_SYMMETRIC_NAT) //&& (TARGET_IPHONE_SIMULATOR)
  if (!isRelayHost(incomingData.remoteAddress()))
  {
    ICELogDebug(<<"Discard packet as symmetric NAT is emulating now");
    return false;
  }
#endif
  
  if (incomingData.remoteAddress().isEmpty())
  {
    ICELogDebug(<< "Incoming packet; remote address is unknown");
    incomingData.setRemoteAddress(NetworkAddress::LoopbackAddress4);
  }
  else
  {
    ICELogDebug(<< "Incoming packet from " << incomingData.remoteAddress().toStdString());
  }
  // Save previous ICE stack state
  int icestate = state();
  
  incomingData.setComponent(component);
  bool result = mSession.processData(incomingData, stream, component);
  
  // Run handlers
  if (result && mEventHandler)
  {
    int newicestate = state();
    if (icestate < IceCheckSuccess && newicestate >= IceCheckSuccess)
    {
      // Connectivity check finished
      if (newicestate == IceCheckSuccess)
        mEventHandler->onSuccess(this, mEventTag);
      else
        mEventHandler->onFailed(this, mEventTag);
    }
    else
    if (icestate < IceGathered && newicestate >= IceGathered)
    {
      // Candidates are gathered
      mEventHandler->onGathered(this, mEventTag);
    }
  }

  return result;
}

PByteBuffer StackImpl::generateOutgoingData(bool& response, int& stream, int& component, void*& tag)
{
  // Get current timestamp
  unsigned timestamp = ICETimeHelper::timestamp();

  // Find amount of spent time
  unsigned spent = ICETimeHelper::findDelta(mActionTimestamp, timestamp);
  
  // Check for timeout
  if (mConfig.mTimeout && spent > mConfig.mTimeout)
  {
    ice::RunningState sessionState = mSession.state();

    bool timeout = sessionState == ice::ConnCheck || sessionState == ice::CandidateGathering;

    if (!mTimeout && timeout)
    {
      // Mark stack as timeouted
      mTimeout = true;
      ICELogInfo(<< "Timeout detected.");
      
      if (sessionState == ice::CandidateGathering)
        mSession.cancelAllocations();
      
      // Find default address amongst host candidates
      mSession.chooseDefaults();

      if (mEventHandler)
      {
        if (sessionState == ice::ConnCheck)
        {
          // Session should not be cleared here - the CT uses direct path for connectivity checks and relayed if checks are failed. Relayed allocations will not be refreshed in such case
          mEventHandler->onFailed(this, this->mEventTag);
        }
        else
          mEventHandler->onGathered(this, this->mEventTag);
      }
    }

    if (timeout)
    {
      // Check if keepalive transactions are scheduled
      if (!mSession.hasAllocations())
        return PByteBuffer();
    }
  }

  // No timeout, proceed...
  PByteBuffer result = mSession.getDataToSend(response, stream, component, tag);
  if (result)
    ICELogInfo(<< "Sending: " << result->comment() << " to " << result->remoteAddress().toStdString()  << ". Data: \r\n" << result->hexstring());
  
  return result;
}

void StackImpl::gatherCandidates()
{
  mActionTimestamp = ICETimeHelper::timestamp();
  mSession.gatherCandidates();

  // If there is no STUN server set or IP6 only interfaces - the candidate gathering can finish without network I/O
  if (mSession.state() == CreatingSDP && mEventHandler)
    mEventHandler->onGathered(this, mEventTag);
}

void StackImpl::checkConnectivity()
{
  // Connectivity check can work if candidate gathering timeout-ed yet - it relies on host candidates only in this case
  // So timeout flag is reset
  mTimeout = false;
  mActionTimestamp = ICETimeHelper::timestamp();
  mSession.checkConnectivity();
}

void StackImpl::stopChecks()
{
  mTimeout = false;
  mActionTimestamp = 0;
  mSession.stopChecks();
}

IceState  StackImpl::state()
{
  if (mTimeout)
    return IceTimeout;

  switch (mSession.state())
  {
  case ice::None:                 return IceNone;
  case ice::CandidateGathering:   return IceGathering;
  case ice::CreatingSDP:          return IceGathered;
  case ice::ConnCheck:            return IceChecking;
  case ice::Failed:               return IceFailed;
  case ice::Success:              return IceCheckSuccess;
  default:
    break;
  }
  
  assert(0);
  return IceNone; // to avoid compiler warning
}

void StackImpl::createSdp(std::vector<std::string>& commonPart)
{
  mSession.createSdp(commonPart);
}

NetworkAddress StackImpl::defaultAddress(int stream, int component)
{
  return mSession.defaultAddress(stream, component);
}

void StackImpl::fillCandidateList(int streamID, int componentID, std::vector<std::string>& candidateList)
{
  mSession.fillCandidateList(streamID, componentID, candidateList);
}

bool StackImpl::processSdpOffer(int streamIndex, std::vector<std::string>& candidateList,
                                 const std::string& defaultIP, unsigned short defaultPort, bool deleteRelayed)
{
  return mSession.processSdpOffer(streamIndex, candidateList, defaultIP, defaultPort, deleteRelayed);
}

NetworkAddress StackImpl::getRemoteRelayedCandidate(int stream, int component)
{
  return mSession.getRemoteRelayedCandidate(stream, component);
}

NetworkAddress StackImpl::getRemoteReflexiveCandidate(int stream, int component)
{
  return mSession.getRemoteReflexiveCandidate(stream, component);
}


void StackImpl::setRemotePassword(const std::string& pwd, int streamId)
{
  mSession.setRemotePassword(pwd, streamId);
}

std::string StackImpl::remotePassword(int streamId) const
{
  return mSession.remotePassword(streamId);
}

void StackImpl::setRemoteUfrag(const std::string& ufrag, int streamId)
{
  mSession.setRemoteUfrag(ufrag, streamId);
}

std::string StackImpl::remoteUfrag(int streamId) const
{
  return mSession.remoteUfrag(streamId);
}

std::string StackImpl::localPassword() const
{
  return mSession.mLocalPwd;
}

std::string StackImpl::localUfrag() const
{
  return mSession.mLocalUfrag;
}

bool StackImpl::hasTurnPrefix(unsigned short prefix) 
{
	return mSession.hasTurnPrefix((TurnPrefix)prefix);
}

NetworkAddress StackImpl::remoteAddress(int stream, int component)
{
	return mSession.remoteAddress(stream, component);
}

NetworkAddress StackImpl::localAddress(int stream, int component)
{
	return mSession.localAddress(stream, component);
}

bool StackImpl::findConcludePair(int stream, Candidate& local, Candidate& remote)
{
	// Find nominated pair in stream
  return mSession.findConcludePair(stream, local, remote);
}

bool StackImpl::candidateListContains(int stream, const std::string& remoteIP, unsigned short remotePort)
{
	return mSession.mStreamMap[stream]->candidateListContains(remoteIP, remotePort);
}

void StackImpl::dump(std::ostream& output)
{
  mSession.dump(output);
}

bool StackImpl::mustRestart()
{
  return mSession.mustRestart();
}

void StackImpl::clear()
{
  mSession.clear();
  mActionTimestamp = 0;
  mTimeout = false;
}

void StackImpl::clearForRestart(bool localNetworkChanged)
{
  mSession.clearForRestart(localNetworkChanged);
  mActionTimestamp = 0;
  mTimeout = false;
}

void StackImpl::refreshPwdUfrag()
{
  mSession.refreshPwdUfrag();
}

TurnPrefix StackImpl::bindChannel(int stream, int component, const ice::NetworkAddress &target, ChannelBoundCallback* cb)
{
  return mSession.bindChannel(stream, component, target, cb);
}

bool StackImpl::isChannelBindingFailed(int stream, int component, TurnPrefix prefix)
{
  return mSession.isChannelBindingFailed(stream, component, prefix);
}

void StackImpl::installPermissions(int stream, int component, const NetworkAddress &address, InstallPermissionsCallback* cb)
{
  mSession.installPermissions(stream, component, address, cb);
}

void StackImpl::freeAllocation(int stream, int component, DeleteAllocationCallback* cb)
{
  mSession.freeAllocation(stream, component, cb);
}

bool StackImpl::hasAllocations()
{
  return mSession.hasAllocations();
}

int StackImpl::errorCode()
{
  return mSession.errorCode();
}

std::vector<Candidate>* StackImpl::remoteCandidates(int stream)
{
  return mSession.remoteCandidates(stream);
}

NetworkAddress StackImpl::activeStunServer(int stream) const
{
  return mSession.activeStunServer(stream);
}

void StackImpl::setup(const ServerConfig& config)
{
  mConfig = config;
  mSession.mConfig.mServerList4 = config.mServerList4;
  mSession.mConfig.mServerList6 = config.mServerList6;
  
  // Set initial STUN/TURN server IP for case if there will no gathering candidates & selecting fastest server.
  if (mSession.mConfig.mServerList4.size())
    mSession.mConfig.mServerAddr4 = mSession.mConfig.mServerList4.front();
  if (mSession.mConfig.mServerList6.size())
    mSession.mConfig.mServerAddr6 = mSession.mConfig.mServerList6.front();
  mSession.mConfig.mUseIPv4 = config.mUseIPv4;
  mSession.mConfig.mUseIPv6 = config.mUseIPv6;

  if (mConfig.mRelay)
  {
    mSession.mConfig.mTurnPassword = config.mPassword;
    mSession.mConfig.mTurnUsername = config.mUsername;

    mSession.mConfig.mUseTURN = true;
    mSession.mConfig.mUseSTUN = false;
  }
  else
  {
    mSession.mConfig.mUseTURN = false;
    mSession.mConfig.mUseSTUN = true;
  }
  mSession.mConfig.mTimeout = config.mTimeout;

  mSession.setup(mSession.mConfig);
}

bool StackImpl::isRelayHost(const NetworkAddress& remote)
{
  for (unsigned i=0; i<mConfig.mServerList4.size(); i++)
  {
    if (NetworkAddress::isSameHost(mConfig.mServerList4[i], remote))
      return true;
  }
  for (unsigned i=0; i<mConfig.mServerList6.size(); i++)
  {
    if (NetworkAddress::isSameHost(mConfig.mServerList6[i], remote))
      return true;
  }
  
  return false;
}

bool StackImpl::isRelayAddress(const NetworkAddress& remote)
{
  for (unsigned i=0; i<mConfig.mServerList4.size(); i++)
  {
    if (mConfig.mServerList4[i] == remote)
      return true;
  }
  for (unsigned i=0; i<mConfig.mServerList6.size(); i++)
  {
    if (mConfig.mServerList6[i] == remote)
      return true;
  }
  
  return false;
}
