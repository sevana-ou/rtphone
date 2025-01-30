/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_SESSION_H
#define __ICE_SESSION_H

#include <vector>
#include <map>
#include <set>

#include "ICEStunConfig.h"
#include "ICECandidate.h"
#include "ICEStunMessage.h"
#include "ICEBinding.h"
#include "ICERelaying.h"
#include "ICESync.h"
#include "ICECheckList.h"
#include "ICETime.h"
#include "ICEStream.h"

namespace ice
{
  // ICE session context
  struct Session
  {
  public:
    Mutex                      mGuard;               // Mutex to protect this instance
    typedef std::map<int, ICEStreamPtr> ICEStreamMap;
    ICEStreamMap                  mStreamMap;           // Streams
    RunningState                  mState;               // State of session
    StackConfig                     mConfig;              // Configuration
    
    size_t                        mStartInterval;       /// Start interval
    unsigned short                mLocalPortNumber;     /// Local port number
    std::string                   mLocalPwd,
                                  mLocalUfrag,
                                  mRemotePwd,
                                  mRemoteUfrag;
    
    NetworkAddress                    mRemoteDefaultAddr;   /// Remote default address
    AgentRole                     mRole;                /// Marks if agent is Controlled or Controlling
    std::string                   mTieBreaker;          /// Tie breaker
    unsigned int                  mFoundationGenerator; /// Foundation value generator. Used when foundation of remote candidate is not available.
    bool                          mCanTransmit;         /// Marks if transmission can start
    bool                          mMustReInvite;        /// Must reINVITE
    TurnPrefixMap                 mTurnPrefixMap;       /// Map of established relays
    bool                          mMustRestart;         /// Marks if session has to restart
    
    /*! Default constructor. */
    Session();
    
    /*! Destructor. */
    ~Session();

    /*! Adjusts session to work with specified component number and specified local port number. */
    void setup(StackConfig& config);

    /*! Checks if ICE session is started. */
    bool active();

    /*! Initiates candidate gathering. Setup() must be called before. */
    void gatherCandidates();
    
    /*! Processing incoming data.
     *  @return True if data were processed, false otherwise (too old response or not STUN data at all). */
    bool processData(ByteBuffer& buffer, int stream, int component);
    
    /*! Checks if there is any data to send. 
     *  @return True if more data to send are available, false if all available data are returned in current call. */
    PByteBuffer getDataToSend(bool& response, int& stream, int& component, void*&tag);
    

    /*! Handles incoming data in gathering candidate stage. 
     *  @return True if data was handled, false otherwise (too old response or not STUN data at all. */
    bool Handle_CG_In(StunMessage& msg, NetworkAddress& address);
    
    /*! Handle eliminate redudand stage. */
    void Handle_ER();
    
    // Compute foundations
    void Handle_CF();
    
    // Starting keep alive timers
    void Handle_SKA();

    // Prioritize candidates
    void Handle_PC();
    
    // Choosing default candidate
    void chooseDefaults();

    // Creation of common SDP strings
    void createSdp(std::vector<std::string>& common);
    
    // Creation of component SDP strings
    void createSdp(int streamID, std::vector<std::string>& defaultIP, 
                         std::vector<unsigned short>& defaultPort, std::vector<std::string>& candidateList);

    bool              findCandidate(std::vector<Candidate>&cv, Candidate::Type _type, 
                        const std::string& baseIP, int componentID, Candidate& result);

    RunningState      state();
    
    std::string       localUfrag();
    std::string       localPwd();
    
    /*! Process ICE offer text. It does not mean SIP offer. No, this method applies to initial ICE candidate list and other information in both SIP offer and answer packets. */
    bool              processSdpOffer(int streamIndex, std::vector<std::string>& candidateList,
                          const std::string& defaultIP, unsigned short defaultPort, bool deleteRelayed);
    NetworkAddress        getRemoteRelayedCandidate(int stream, int component);
    NetworkAddress        getRemoteReflexiveCandidate(int stream, int component);

    NetworkAddress        defaultAddress(int streamID, int componentID);
    void              fillCandidateList(int streamID, int componentID, std::vector<std::string>& candidateList);

    /*! Seeks in candidate list for candidate with specified IP and port. */
    bool              candidateListContains(std::string remoteIP, unsigned short remotePort);
    
    /*! Marks agent as Controlled or Controlling role. */
    void              setRole(AgentRole role);
    
    /*! Gets current role. */
    AgentRole         role();

    /*! Binds channel to prefix */
    TurnPrefix        bindChannel(int stream, int component, const NetworkAddress& target, ChannelBoundCallback* cb);
    bool              isChannelBindingFailed(int stream, int component, TurnPrefix prefix);

    void              installPermissions(int stream, int component, const NetworkAddress &address, InstallPermissionsCallback* cb);

    /*! Enqueues CreatePermisssion transaction with zero lifetime - it removes permission. */
    void              deletePermission(int stream, int component, DeletePermissionsCallback* cb);
    
    // Enqueues ClientAllocate with zero lifetime
    void              freeAllocation(int stream, int component, DeleteAllocationCallback* cb);
    
    // Returns if there were any TURN allocations from this stack
    bool              hasAllocations();
        
    static bool       isDataIndication(ByteBuffer& source, ByteBuffer* plain);
    static bool       isStun(const ByteBuffer& source);
    static bool       isRtp(ByteBuffer& source);
    static bool       isChannelData(ByteBuffer& source, TurnPrefix prefix);

    /*! Starts connectivity checks. */
    void              checkConnectivity();
    
    // Clears state of session and streams; does not delete streams or components. Stops all connectivity checks. Candidates must be gathered again.
    void              clear();
    
    void              clearForRestart(bool localNetworkChanged);
    
    // Returns true if state of session is Success or Failed
    bool              finished();
    
    // Stop connectivity checks and gathering requests.
    void              stopChecks();
    
    // Cancel allocation requests. Called when timeout is detected and allocation should be cancelled.
    // Allocation transaction and session can have different timeout values - so this method is neccessary
    void cancelAllocations();
    
    /*! Returns concluding addresses . */
    NetworkAddress remoteAddress(int streamID, int componentID);
    NetworkAddress localAddress(int streamID, int componentID);
    
    /*! Searches for local server reflexive candidate with specified component ID and returns its external address. */
    NetworkAddress reflexiveAddress(int streamID, int componentID);
    
    /*! Searches for local server relayed candidate with specified component ID and returns its external address. */
    NetworkAddress relayedAddress(int streamID, int componentID);

    /*! Checks if argument is used TURN prefix in one of the TURN bound channels. */
    bool hasTurnPrefix(TurnPrefix prefix);

    /*! Adds stream to session. Returns stream index. */
    int addStream();

    /*! Adds component to stream.
     *  @param streamIndex Stream index.
     *  @param tag Tag associated with component.
     *  @return Created component record index. */
    int addComponent(int streamID, void *tag, unsigned short port4, unsigned short port6);

    /* Removes stream with specified ID. All stream's components are removed too. */
    void removeStream(int streamID);

    /* Searches the stream&component ID basing on socket family (AF_INET or AF_INET6) and local port number. */
    bool findStreamAndComponent(int family, unsigned short port, int* stream, int* component);

	  bool hasStream(int streamId);
    bool hasComponent(int streamId, int componentId);
	  void setComponentPort(int streamId, int componentId, unsigned short port4, unsigned short port6);

    /*! Generates new ICE ufrag string. */
    static std::string  createUfrag();
    
    /*! Generates new ICE pwd string. */
    static std::string  createPassword();

    void setRemotePassword(const std::string& pwd, int streamId = -1);
    std::string remotePassword(int streamId = -1) const;
    void setRemoteUfrag(const std::string& ufrag, int streamId = -1);
    std::string remoteUfrag(int streamId = -1) const;
    
    void refreshPwdUfrag();

    void dump(std::ostream& output);
    bool mustRestart() const;
    bool findConcludePair(int stream, Candidate& local, Candidate& remote);
    int errorCode();
    Stream::CandidateVector* remoteCandidates(int stream);

    NetworkAddress activeStunServer(int stream) const;
  };
};

#endif
