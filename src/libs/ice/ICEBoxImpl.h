/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_BOX_IMPL__H
#define __ICE_BOX_IMPL__H

#include "ICEBox.h"
#include "ICESession.h"
#include "ICELog.h"

namespace ice
{
  class StackImpl: public Stack
  {
  protected:
    ServerConfig         mConfig;
    Mutex                mGuard;                 //mutex to protect ICE stack object
    Session              mSession;
    StageHandler*        mEventHandler;
    void*                   mEventTag;
    bool                    mTimeout;
    unsigned int            mActionTimestamp;
		
		void logMsg(LogLevel level, const char* msg);
		
  public:
      
    StackImpl(const ServerConfig& config);
    ~StackImpl();

    void          setEventHandler(StageHandler* handler, void* tag) override;
    int           addStream() override;
    int           addComponent(int streamID, void* tag, unsigned short port4, unsigned short port6) override;
    void          removeStream(int streamID) override;
    bool          findStreamAndComponent(int family, unsigned short port, int* stream, int* component) override;
	  bool		      hasStream(int streamId) override;
    bool          hasComponent(int streamId, int componentId) override;
	  void		      setComponentPort(int streamId, int componentId, unsigned short port4, unsigned short port6) override;
		
    void          setRole(AgentRole role) override;
    AgentRole     role() override;
		
    bool          processIncomingData(int stream, int component, ByteBuffer& incomingData) override;
    PByteBuffer   generateOutgoingData(bool& response, int& stream, int& component, void*& tag) override;

    // Attempt to gather candidates for specified channel
    void          gatherCandidates() override;
    void          checkConnectivity() override;
    void          stopChecks() override;
    void          restartCheckConnectivity();

    IceState      state() override;
    
    void          createSdp(std::vector<std::string>& commonPart) override;
    NetworkAddress    defaultAddress(int streamID, int componentID) override;
    void          fillCandidateList(int streamID, int componentID, std::vector<std::string>& candidateList) override;

    bool          processSdpOffer(int streamIndex, std::vector<std::string>& candidateList,
                                  const std::string& defaultIP, unsigned short defaultPort, bool deleteRelayed) override;
    NetworkAddress  getRemoteRelayedCandidate(int stream, int component) override;
    NetworkAddress  getRemoteReflexiveCandidate(int stream, int component) override;

    void          setRemotePassword(const std::string& pwd, int streamId = -1) override;
    std::string   remotePassword(int streamId = -1) const override;
    void          setRemoteUfrag(const std::string& ufrag, int streamId = -1) override;
    std::string   remoteUfrag(int streamId = -1) const override;

    std::string   localPassword() const override;
    std::string   localUfrag() const override;
    bool					hasTurnPrefix(unsigned short prefix) override;
	  NetworkAddress		remoteAddress(int stream, int component) override;
	  NetworkAddress    localAddress(int stream, int component) override;
    bool					findConcludePair(int stream, Candidate& local, Candidate& remote) override;
    bool					candidateListContains(int stream, const std::string& remoteIP, unsigned short remotePort) override;
    void          dump(std::ostream& output) override;
    bool          mustRestart() override;
    void          clear() override;
    void          clearForRestart(bool localNetworkChanged) override;
    void          refreshPwdUfrag() override;

    // Channel binding
    TurnPrefix    bindChannel(int stream, int component, const NetworkAddress& target, ChannelBoundCallback* cb) override;
    bool          isChannelBindingFailed(int stream, int component, TurnPrefix prefix) override;

    // Permissions
    void          installPermissions(int stream, int component, const NetworkAddress& address, InstallPermissionsCallback* cb) override;

    // Allocations
    void          freeAllocation(int stream, int component, DeleteAllocationCallback* cb) override;
    bool          hasAllocations() override;


    int           errorCode() override;
    std::vector<Candidate>* 
                  remoteCandidates(int stream) override;
    NetworkAddress    activeStunServer(int stream) const override;
    void          setup(const ServerConfig& config) override;
    bool          isRelayHost(const NetworkAddress& remote) override;
    bool          isRelayAddress(const NetworkAddress& remote) override;
  };
} //end of namespace

#endif
