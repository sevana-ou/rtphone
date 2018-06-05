/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_STREAM_H
#define __ICE_STREAM_H

#include <map>
#include <vector>

#include "ICEBox.h"
#include "ICECandidate.h"
#include "ICEStunConfig.h"
#include "ICEAction.h"
#include "ICEStunTransaction.h"
#include "ICECheckList.h"
#include "ICEBinding.h"
#include "ICETime.h"
#include "ICETransactionList.h"
#include "ICERelaying.h"

namespace ice
{
  enum Failover
  {
    FailoverOn = 1,
    FailoverOff = 2
  };

  // Session state
  enum RunningState
  {
    None = 0,
    CandidateGathering,
    EliminateRedudand,
    ComputingFoundations,
    StartingKeepAlives,
    PrioritizingCandidates,
    ChoosingDefault,
    CreatingSDP,
    ConnCheck,
    Failed,
    Success
  };

  extern const char* RunningStateToString(RunningState state);

  // Smart pointer to ICE stream type
  typedef SmartPtr<Stream> ICEStreamPtr;
  
  
  // Map of used channel TURN prefixes as key and corresponding address as value
  typedef std::map<TurnPrefix, NetworkAddress> TurnPrefixMap;

  struct Component
  {
    void* mTag;
    unsigned short mPort4;
    unsigned short mPort6;
    unsigned mNominationWaitIntervalStartTime;
    Component()
      :mTag(NULL), mPort4(0), mPort6(0), mNominationWaitIntervalStartTime(0)
    {}

    ~Component()
    {
    }
  };

  typedef std::map<int, Component>   ComponentMap;

  // Represents single ICE stream with single or multiple components (sockets)
  struct Stream
  {
    // Stream id
    int mId;

    // Stack ID. Used for debugging purposes.
    int mStackId;

    // Map of component ID -> user tag
    ComponentMap mComponentMap;

    // List of local candidates and list of remote candidates
    typedef std::vector<Candidate> CandidateVector;
    CandidateVector   mLocalCandidate, mRemoteCandidate, mRemoteRelayedCandidate;

    // Check list
    CheckList mCheckList;
    
    // Logger
    Logger*        mLogger;

    // Foundation generator value to provide foundation value for peer reflexive candidates
    unsigned int      mFoundationGenerator;

    // Active STUN transactions. This vector includes connectivity check transactions and gather candidates transactions.
    // Keepalive transactions are stored in mKeepAliveList
    TransactionList   mActiveChecks;

    // ICE agent role
    AgentRole         mAgentRole;

    // Map of established relay channels
    TurnPrefixMap     mTurnPrefixMap;       
    
    // Current state of media stream
    RunningState      mState;
    
    // Used configuration
    StackConfig         mConfig;
    
    // Helper vector used to prioritize candidates
    std::vector<unsigned>     mComponentLimit;

    // Default candidate list - one per each component
    std::map<int, Candidate>  mDefaultCandidate;
    
    // Local password/ufrag
    std::string       mLocalPwd;
    std::string       mLocalUfrag;

    // Remote password/ufrag
    std::string       mRemotePwd;
    std::string       mRemoteUfrag;

    // Marks if selected during connectivity checks default IP list differs from used to generate offer
    bool              mDefaultIPChanged;
    
    // Marks if checks was ok for this stream and each component has valid nominated pair
    bool              mCanTransmit;
    
    std::vector<ByteBuffer*> mResponseQueue;
    // Tie breaker
    std::string       mTieBreaker;

    // Timer to schedule connection checks (CC)
    ICEScheduleTimer  mScheduleTimer;

    // Counter of TURN allocations
    int               mTurnAllocated;
    
    // Last error code during gathering/checks
    int               mErrorCode;

    // Cached realm and nonce for used TURN server
    std::string       mCachedRealm, mCachedNonce;
    
    // Timestamp of nomination waiting timer. Used to accumulate valid pairs and chose the "best" of them.
    unsigned          mNominationWaitStartTime;
    
    // Number of finished failover gathering requests
    int               mFailoverRequestsFinished;
    
    // Failover ID generator. Failover transactions which share the same goal share the same ID.
    int               mFailoverIdGenerator;
    
    struct BoundChannel
    {
      int mComponentId;
      TurnPrefix mPrefix;
      NetworkAddress mPeerAddress;
      int mResultCode;
    };
    typedef std::vector<BoundChannel> BoundChannelList;
    BoundChannelList mBoundChannelList;
    
    // Create Allocate transaction and associate with passed action
    struct AllocateOptions
    {
      NetworkAddress mServerAddress;
      PAction mActionOnFinish;
      int mComponent = 0;
      Failover mFailoverOption = FailoverOn;
      int mWireFamily = AF_INET;
      int mAllocFamily = AF_INET;
      int mFailoverId = 0;
    };
    void allocate(const AllocateOptions& options);
    
    // Create Bind transaction and associate with passed action
    void              bind(const NetworkAddress& addr, PAction action, bool auth, int component, Failover failover);
    
    // Searches for local candidate with specified address
    unsigned          findLocalCandidate(const NetworkAddress& addr);
    
    // Cancels found transaction.
    void              cancelCheck(CandidatePair& p);
    
    // Cancels allocations
    void              cancelAllocations();
    
    // Searches for remote candidate with specified remote address and corresponding to local port number
    unsigned          findRemoteCandidate(const NetworkAddress& addr, int componentID);

    // Process incoming binding request
    void              handleBindingRequest(ServerBinding& binding, ByteBuffer& buffer, int component);

    // Handles incoming data in gathering candidate stage. 
    bool              handleCgIn(StunMessage& msg, NetworkAddress& address);
    
    // Handle eliminate redudand stage
    void              Handle_ER();
    
    // Compute foundations
    void              Handle_CF();
    
    // Starting keep alive timers
    void              Handle_SKA();

    // Prioritize candidates
    void              Handle_PC();
    
    // Choosing default candidate
    void              Handle_CD();
    
    // Handles keepalive messages
    //bool              handleKeepAlivesIn(StunMessage& msg, NetworkAddress& address);

    // Handles incoming messages in connectivity checks stage
    // This method uses Handle_KA_In to perform keepalive handling
    bool              handleConnChecksIn(StunMessage& msg, NetworkAddress& address);

    // Handle incoming Bind request
    void              handleIncomingRequest(StunMessage& msg, ByteBuffer& buffer, int component);

    // Chooses default candidate for specified component ID
    Candidate      findDefaultCandidate(int componentID);

    //Performs 8.1.2. Updating States
    void              checkNominated(int componentID);
    void              checkNominated();
    bool              handleRoleConflict(ServerBinding& binding);
    
    //Checks active transactions (keepalive and ordinary) for timeouts
    void              isTimeout();

    // Checks for next byte buffer for sending
    ByteBuffer*    handleConnChecksOut();
    
    // Creates connectivity check request for specified pair.
    Transaction*      createCheckRequest(PCandidatePair& p);

    // Checks for TURN channel prefix by specified peer's address. 
    // Returns zero prefix if it is not found.
    TurnPrefix        findTurnPrefixByAddress(NetworkAddress& peerAddress);

    // Create candidate pair in check list.
    void              createCheckList();
    
    // Starts connectivity checks - calls Create_CheckList to create check list + starts retransmission timer
    void              startChecks();

    // Stops connectivity checks and gathering requests
    void              stopChecks();
    
    // Initiates ChannelBind transaction to TURN server. 
    TurnPrefix        bindChannel(const NetworkAddress& peerAddress, int component, ChannelBoundCallback* cb = NULL);
    bool              isChannelBindingFailed(int component, TurnPrefix prefix);
    void              removeBindingResult(int component);
    
    // Attempts to free allocation.
    void              freeAllocation(int component, DeleteAllocationCallback* cb = NULL);

    // Searches for local server reflexive candidate with specified component ID and returns its external address. 
    NetworkAddress        reflexiveAddress(int componentID);

    /*! Searches for local server relayed candidate with specified component ID and returns its external address. */
    NetworkAddress        relayedAddress(int componentID);
    
    // Searches for remote server relayed candidate with specified component
    NetworkAddress        remoteRelayedAddress(int component);

    // Searches for remote server reflexive candidate with specified component
    NetworkAddress        remoteReflexiveAddress(int component);
    
    
    Stream();
    ~Stream();
    
    // Sets config
    void setConfig(StackConfig& config);

    // Sets ICE agent role - Controlled or Controlling
    void setAgentRole(AgentRole role);

    // Sets local password
    void setLocalPwd(const std::string& pwd);
    
    // Sets local ufrag
    void setLocalUfrag(const std::string& ufrag);
    
    // Sets remote password
    void setRemotePwd(const std::string& pwd);
    
    // Sets remote ufrag
    void setRemoteUfrag(const std::string& ufrag);

    // Sets tie breaker
    void setTieBreaker(const std::string& tieBreaker);

    // Adds new component to stream
    int addComponent(void* tag, unsigned short port4, unsigned short port6);

    // Gathers candidates for stream
    void gatherCandidates();
    
    // Returns reference to check list
    CheckList& checkList();

    // Check is stream owns specified port number
    bool hasPortNumber(int family, unsigned short portNumber, int* component = NULL);

    // Processes incoming data
    bool processData(StunMessage& msg, ByteBuffer& buffer, int component);

    void createOfferSdp(std::vector<std::string>& defaultIP, std::vector<unsigned short>& defaultPort, 
                                 std::vector<std::string>& candidateList);
    
    NetworkAddress defaultAddress(int componentID);
    void candidateList(int componentID, std::vector<std::string>& candidateList);

    PByteBuffer getDataToSend(bool& response, int& component, void*&tag);

    // Constructs new keepalive transaction and adds to mKeepAlives
    void addKeepAliveCheck(CandidatePair& p);
    
    // Processes SDP offer, adding remote candidates from it
    bool processSdpOffer(std::vector<std::string>& candidateList, std::string defaultIP, unsigned short defaultPort, bool deleteRelayed);

    // Checks if remote candidate list contains specified address 
    bool candidateListContains(const std::string& remoteIP, unsigned short remotePort);
    
    // Restarts stream's connectivity checks
    void restart();

    // Clears the stack; resets state to None. The free allocation requests are left in the queue
    void clear();

    // Deletes existing connectivity checks and resets turn allocation counters
    void clearForRestart(bool localNetworkChanged);
    
    // Returns resolved address of remote party identified by its componmentID
    NetworkAddress remoteAddress(int component);
    NetworkAddress localAddress(int component);

    void installPermissions(int component = -1, const NetworkAddress& addr = NetworkAddress(), InstallPermissionsCallback* cb = NULL);
    void dump(std::ostream& output);
    bool findConcludePair(Candidate& local, Candidate& remote);
    int findComponentIdByPort(unsigned short port, int family);
    Transaction* runCheckList(CandidatePair::Role role, CandidatePair::State state);
    void deleteTransactionAt(unsigned index);
    void clearChecks();
    
    void dumpLocalCandidateList();
    void processStateChain();
    
    // Disables (removes) active transactions responsible for gathering candidates - binding & allocating
    void removeGatherRequests(int component, int failoverId, Transaction* validOne);
    void unfreeze(const char* foundation);
    void nominatePair(PCandidatePair& p);
    bool ressurectAllocation(int component);
  };
}

#endif
