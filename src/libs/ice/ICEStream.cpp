/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEStream.h"
#include "ICENetworkHelper.h"
#include "ICEBinding.h"
#include "ICERelaying.h"
#include "ICELog.h"
#include "ICESync.h"
#include "ICESession.h"

#include <stdexcept>
#include <strstream>
#include <algorithm>
#include <assert.h>

using namespace ice;

#define LOG_SUBSYSTEM "ICE"

const char* ice::RunningStateToString(RunningState state)
{
  switch(state)
  {
  case None:                    return "None";
  case CandidateGathering:      return "CandidateGathering";
  case EliminateRedudand:       return "EliminateRedudand";
  case ComputingFoundations:    return "ComputingFoundations";
  case StartingKeepAlives:      return "StartingKeepAlives";
  case PrioritizingCandidates:  return "PrioritizingCandidates";
  case ChoosingDefault:         return "ChoosingDefault";
  case CreatingSDP:             return "CreatingSDP";
  case ConnCheck:               return "ConnCheck";
  case Failed:                  return "Failed";
  case Success:                 return "Success";
  }
  return "Undefined";
}

// This class is intended to bring client binding results with candidates
class BindingAction: public Action
{
public:
  BindingAction(Stream& stream, StackConfig& config)
    :Action(stream, config)
  {
  }

  virtual void finished(Transaction& st)
  {
    // Declare possible keepalive binding transaction
    ClientBinding* kaBinding = NULL;

    // Cast finished transaction to ClientBinding
    ClientBinding& cb = dynamic_cast<ClientBinding&>(st);
    
    // Iterate local candidate to update values
    for (unsigned i=0; i<mStream.mLocalCandidate.size(); i++)
    {
      Candidate& c = mStream.mLocalCandidate[i];
      if (c.mType == Candidate::ServerReflexive && c.mComponentId == st.component())
      {
        // Update candidate state
        c.mFailed = cb.state() == Transaction::Failed;
        c.mReady = true;
        if (!c.mFailed)
          c.mExternalAddr = cb.mappedAddress();
      }
    }

    if (cb.state() == Transaction::Success)
    {
      ICELogInfo( << "Stack ID " << st.stackId() << ". Gathered reflexive address " << cb.mappedAddress().toStdString());
      
      // Delete other parallel gathering requests to avoid races
      mStream.removeGatherRequests(st.component(), cb.failoverId(), &st);

      // Set working address
      mStream.mConfig.mServerAddr4 = st.destination();
      
#ifdef ICE_ENABLE_KEEPALIVE
      // Create keepalive transaction (it is usual Binding)
      kaBinding = new ClientBinding();
      kaBinding->setStackId(mStream.mStackId);

      // Set destination address
      kaBinding->setDestination(cb.transportType() == IPv4 ? mConfig.mServerAddr4 : mConfig.mServerAddr6);
        
      // Set source component port number
      kaBinding->setComponent(st.component());
      kaBinding->setKeepalive(true);
      kaBinding->setInterval(mConfig.mKeepAliveInterval);
      mStream.mActiveChecks.addRegular(kaBinding);
#endif
    }
    else
    {
      ICELogCritical( << "Stack ID " << st.stackId() << ". Failed to gather reflexive candidate.");
      mStream.mErrorCode = cb.errorCode();
    }
  }
};


class RelayingAction: public Action
{
protected:
  bool mAutoreleaseAllocation;
  
public:
  RelayingAction(Stream& stream, StackConfig& config)
    :Action(stream, config), mAutoreleaseAllocation(false)
  {
  }

  void autoreleaseAllocation()
  {
    mAutoreleaseAllocation = true;
  }
  
  virtual void finished(Transaction& transaction)
  {
    ClientAllocate& ca = dynamic_cast<ClientAllocate&>(transaction);
    
    // Increase counter of finished Allocate requests
    mStream.mFailoverRequestsFinished++;
    ICELogDebug(<< "Increase failover gathering finished requests counter to " << mStream.mFailoverRequestsFinished);
    
    // Check if this Allocation must be freed ASAP
    if (mAutoreleaseAllocation)
    {
      if (ca.state() == Transaction::Success)
      {
        ICELogInfo(<< "Has to free this allocation as it is not needed due to failover scheme");
        // Increase counter of turn allocation for stream. It will be decreased again in ClientRefresh::processSuccessMessage
        mStream.mTurnAllocated++;
        
        // Send deallocation message
        ClientRefresh* allocation = new ClientRefresh(0, &mStream, &ca);
        allocation->setKeepalive(false);
        allocation->setDestination(ca.destination());
        mStream.mActiveChecks.addRegular(allocation);
      }
      return;
    }
    
    // Save latest credentials from server
    if (ca.realm().size() && ca.nonce().size())
    {
      mStream.mCachedRealm = ca.realm();
      mStream.mCachedNonce = ca.nonce();
    }
    
    // Update local candidates
    for (unsigned i=0; i<mStream.mLocalCandidate.size(); i++)
    {
      Candidate& c = mStream.mLocalCandidate[i];
      
      // Avoid updating of successful candidates to failed state
      if (c.mComponentId == ca.component() && (!c.mReady || c.mFailed))
      {
        c.mReady = true;
        c.mFailed = ca.state() == Transaction::Failed;
        if (!c.mFailed)
        {
          if (c.mType == Candidate::ServerReflexive)
            c.mExternalAddr = ca.reflexiveAddress();
          else
          if (c.mType == Candidate::ServerRelayed)
            c.mExternalAddr = ca.relayedAddress();
        }
      }
    }

    if (ca.state() == Transaction::Success)
    {
      ICELogInfo( << "Stack ID " << transaction.stackId() << ". Gathered reflexive address " << ca.reflexiveAddress().toStdString() << " and relayed address " << ca.relayedAddress().toStdString());
      
      // Remove other parallel requests sent to be failover
      mStream.removeGatherRequests(transaction.component(), transaction.failoverId(), &transaction);
      
      // Set working address
      mStream.mConfig.mServerAddr4 = transaction.destination();
      
      // Create keepalive transaction (cast lifetime parameter to milliseconds)
#ifdef ICE_ENABLE_KEEPALIVE
      ClientRefresh* cf = new ClientRefresh( ca.lifetime(), &mStream, &ca );
      // Set 5 seconds interval to avoid NAT problems
      cf->setInterval(5);
      //cf->setInterval( ca.lifetime() / 2);
      cf->setKeepalive( true );
      cf->setType( Transaction::KeepAlive );
      cf->setDestination(ca.destination());
      
      // Defer transaction run to interval() value
      cf->setTimestamp( ICETimeHelper::timestamp() ); 

      // Increase counter of TURN allocations
      mStream.mTurnAllocated++;

      // Associate Refresh transaction with retransmission timer
      mStream.mActiveChecks.addRegular(cf);
#endif
    }
    else
    {
      ICELogCritical( << "Stack ID " << transaction.stackId() << ". Failed to gather reflexive/relayed addresses.");
      bool ressurected = false;
      // Check if single server is used and error code is 437 - in this case attempt to recover old Allocation will be made
      if (mStream.mConfig.mServerList4.size() < 2 && ca.errorCode() == 437)
      {
        if (true == (ressurected = mStream.ressurectAllocation(transaction.component())))
        {
          // Increase counter of TURN allocations
          mStream.mTurnAllocated++;
          
          ICELogCritical(<< "Stack ID " << transaction.stackId() << ". Ressurected old Refresh transaction. Error will not be raized.");
        }
      }
      
      if (!ressurected)
        mStream.mErrorCode = ca.errorCode();

      // There is no any further processing here - result of transaction will be handled later
    }
  }
};

class CheckPairAction: public Action
{
  friend struct Stream;
public:
  CheckPairAction(Stream& stream, StackConfig& config, PCandidatePair& p)
   :Action(stream, config), mPair(p), mNomination(false), mAgentRole(stream.mAgentRole)
  {
  }

  virtual ~CheckPairAction()
  {
  }

  virtual void finished(Transaction& transaction)
  {
    CheckResult& cr = dynamic_cast<CheckResult&>(transaction);

    //ConnectivityCheck& cb = dynamic_cast<ConnectivityCheck&>(transaction);
    
    bool success = false;
    if (cr.resultState() == Transaction::Failed)
    {
	  // Handle role conflict
      if (cr.resultError() == 487)
      {
        // Change session role
        if (mAgentRole == RoleControlling)
          mStream.mAgentRole = RoleControlled;
        else
          mStream.mAgentRole = RoleControlling;

        transaction.restart();
        return;
      }
    }
    else
      success = true;
    
    // A check is considered to be a success if all of the following are
    // true:
    //- the STUN transaction generated a success response
    success &= cr.resultState() == Transaction::Success;

    if (success && mNomination &&
        mPair->nomination() != CandidatePair::Nomination_Finished)
    {
      ICELogInfo(<< "Received response for nominated request");
      
      //  Add keep-alive check
      mStream.addKeepAliveCheck(*mPair);
    
      // Nomination is finished for this pair
      mPair->setNomination(CandidatePair::Nomination_Finished);

      // Ensure RTCP component is unfrozen
      mStream.unfreeze(mPair->foundation());
      
      // Maybe processing for component is finished?
      mStream.checkNominated();

      return;
    }
    
    // Reset transaction tag for checked pair
    mPair->setTransaction(NULL);

    ICELogInfo ( << "Stack ID " << transaction.stackId() << ". Check " <<  mPair->toStdString() << " is " << (success ? "ok" : "failed") << ".");

    //- the source IP address and port of the response equals the
    //  destination IP address and port that the Binding Request was sent
    //  to
    if (success)
      success &= (cr.resultSource() == mPair->second().mExternalAddr);
    
    //- the destination IP address and port of the response match the
    //  source IP address and port that the Binding Request was sent from
    mPair->setState(success ? CandidatePair::Succeeded : CandidatePair::Failed);

    if (success)
    {
      std::string foundation = mPair->foundation();
      mStream.dumpLocalCandidateList();
      unsigned li = mStream.findLocalCandidate( cr.resultLocal() );
      ICELogDebug( << "Looking for " << cr.resultLocal().toStdString() << " in local candidate list. Result index is " << int(li) );
      if ( li == NoIndex )
      {
        // Add peer reflexive candidate
        ICELogInfo(<< "Add peer reflexive candidate " << cr.resultLocal().toStdString());
        
        // Its type is equal to peer reflexive.
        Candidate cand(Candidate::PeerReflexive);
        cand.mExternalAddr = cr.resultLocal();

        // Its base is set equal to the local candidate of the candidate pair from which the STUN check was sent.
        cand.mLocalAddr = mPair->first().mLocalAddr;
        
        // Its priority is set equal to the value of the PRIORITY attribute in the Binding Request.
        cand.mPriority = cr.resultPriority();

        // Its foundation is selected as described in Section 4.1.1.
        cand.computeFoundation();
        
        // This peer reflexive candidate is then added to the list of local
        // candidates for the media stream
        mStream.mLocalCandidate.push_back(cand);

        // The agent constructs a candidate pair whose local candidate equals
        // the mapped address of the response, and whose remote candidate equals
        // the destination address to which the request was sent.
        CandidatePair _pair;
        
        // Use created local peer reflexive candidate
        _pair.first() = cand;
        
        // Create new remote candidate
        Candidate& newRemoteCand = _pair.second();
        newRemoteCand.mLocalAddr = cr.resultSource();
        newRemoteCand.mExternalAddr = cr.resultSource();
        newRemoteCand.mPriority = cr.resultPriority();
        newRemoteCand.computeFoundation();

        _pair.updateFoundation();
        _pair.updatePriority();
        _pair.setRole(CandidatePair::Valid);
        mStream.checkList().add(_pair);
        
        // Ensure reference to original ICECandidatePair is valid yet
        ICELogInfo( << "Stack ID " << transaction.stackId() << ". Created peer reflexive candidate and corresponding pair " << _pair.toStdString() <<  " in valid list.");
      }
      else
      {
        ICELogInfo( << "Stack ID " << transaction.stackId() << ". Pair " << mPair->toStdString() <<" is add to valid list.");
        mPair->setRole(CandidatePair::Valid); // And this pair is valid now
      }
      
      // Now - unfreeze all pairs from the same queue and same foundation
      mStream.unfreeze(foundation.c_str());
      mPair->setTransaction(&transaction);
      
      // Start regular nomination
      if (mStream.mAgentRole == RoleControlling && !mNomination)
      {
        Component& component = mStream.mComponentMap[transaction.component()];

        // If remote address is not LAN - maybe it should be delayed to give chance
        // to finish LAN -> LAN checks before.
        // ICE_NOMINATION_WAIT_INTERVAL is responsible for this
        if (!mPair->second().mExternalAddr.isLAN() && ICE_NOMINATION_WAIT_INTERVAL != 0 && mStream.mState != Success)
        {
          // Check if
          if (!component.mNominationWaitIntervalStartTime)
          {
            component.mNominationWaitIntervalStartTime = ICETimeHelper::timestamp();
            return;
          }
          else
          {
            unsigned currentTime = ICETimeHelper::timestamp();
            if (ICETimeHelper::findDelta(component.mNominationWaitIntervalStartTime, currentTime) >= ICE_NOMINATION_WAIT_INTERVAL)
              mNomination = true; // Start nomination
          }
        }
        else
          mNomination = true;

        if (mNomination && mStream.mState != Success)
        {
          component.mNominationWaitIntervalStartTime = 0; // Stop nomination interval timer
          mStream.nominatePair(mPair);
        }
      }
    }
    else
      mPair->setState(CandidatePair::Failed);
  }

  PCandidatePair& getPair()
  {
    return mPair;
  }

  void setNomination(bool value)
  {
    mNomination = value;
  }

  bool nomination() const
  {
    return mNomination;
  }

protected:
  PCandidatePair        mPair;        /// Pair
  bool                  mNomination;  /// Nomination request flag
  int                   mAgentRole;
};



//This class is intended to bring client binding results with candidates
class ICEInstallPermissionsAction: public Action
{
protected:
  InstallPermissionsCallback* mCallback;

public:
  ICEInstallPermissionsAction(Stream& stream, StackConfig& config, InstallPermissionsCallback* cb)
    :Action(stream, config), mCallback(cb)
  {
  }

  ~ICEInstallPermissionsAction()
  {
    delete mCallback;
  }

  virtual void finished(Transaction& st)
  {
    ClientCreatePermission& ccp = dynamic_cast<ClientCreatePermission&>(st);
    int code = 0;

    if (st.state() == Transaction::Success)
    {
      ClientCreatePermission::IpList al = ccp.ipList();
      std::string iptext;
      for (unsigned i=0; i<al.size(); i++)
        iptext += al[i].ip() + " ";
      ICELogInfo(<<"TURN client permissions are installed for: " << iptext);
    }
    else
    {
      ICELogCritical(<<"Failed to install TURN client permissions.");
      AuthTransaction& auth = dynamic_cast<AuthTransaction&>(st);
      code = auth.errorCode() ? auth.errorCode() : -1;
    }
    if (mCallback)
      mCallback->onPermissionsInstalled(mStream.mId, st.component(), code);
  }
};

static long GStackID = 0;

Stream::Stream()
{
  mErrorCode = 0;
  mState = None;
  mFoundationGenerator = 0xFFFFFFFF;
  mAgentRole = RoleControlling;
  mDefaultIPChanged = false;
  mCanTransmit = false;
  
  mStackId = Atomic::increment(&GStackID);
  mTurnAllocated = 0;
  mFailoverRequestsFinished = 0;
  mFailoverIdGenerator = 0;
}

Stream::~Stream()
{
}

void Stream::setConfig(StackConfig& config)
{
  mCachedNonce.clear();
  mCachedRealm.clear();
  mConfig = config;
}

int Stream::addComponent(void* tag, unsigned short port4, unsigned short port6)
{
  int componentID = mComponentMap.size() + 1;

  Component& c = mComponentMap[componentID];
  c.mTag = tag;
  c.mPort4 = port4;
  c.mPort6 = port6;

  return componentID;
}
void Stream::setAgentRole(AgentRole role)
{
  mAgentRole = role;
}

void Stream::setLocalPwd(const std::string& pwd)
{
  mLocalPwd = pwd;
}
    
void Stream::setLocalUfrag(const std::string& ufrag)
{
  mLocalUfrag = ufrag;
}

void Stream::setRemotePwd(const std::string& pwd)
{
  mRemotePwd = pwd;
}
    
void Stream::setRemoteUfrag(const std::string& ufrag)
{
  mRemoteUfrag = ufrag;
}

void Stream::setTieBreaker(const std::string& tieBreaker)
{
  mTieBreaker = tieBreaker;
}

void Stream::gatherCandidates()
{
  ICELogInfo (<<"Stack ID " << mStackId << ". Attempt to gather candidates. Use IPv4 = " << mConfig.mUseIPv4 << ", IPv6 = " << mConfig.mUseIPv6);
  mState = CandidateGathering;
  mErrorCode = 0;
  mFailoverRequestsFinished = 0;
  mLocalCandidate.clear();
  mDefaultCandidate.clear();
  
  // Get list of available IP interfaces
  Lock l(NetworkHelper::instance().guard());
  std::vector<NetworkAddress>& interfaceList = NetworkHelper::instance().interfaceList();
  for (auto& addr: interfaceList)
    ICELogDebug(<< "    " << addr.toStdString());
  
  // See if there is public IP address
  // Iterate components
  
  int numberOfActions = 0;
  
  ComponentMap::iterator componentIter;
  for (componentIter = mComponentMap.begin(); componentIter != mComponentMap.end(); ++componentIter)
  {
    ICELogDebug(<< "Checking ICE component " << componentIter->first);
    BindingAction* bindingAction = nullptr;
    RelayingAction *relayingAction = nullptr, /**getIp6Action = nullptr,*/ *getIp4Action = nullptr;

    // Clear binding results as new allocation will exists anyway
    removeBindingResult(componentIter->first);

    // Iterate available IP interfaces i.e. local IP addresses
    for (unsigned interfaceIndex = 0; interfaceIndex < interfaceList.size(); interfaceIndex++)
    {
      NetworkAddress& ipInterface = interfaceList[interfaceIndex];
      if (ipInterface.isLoopback() || ipInterface.isLinkLocal() || ipInterface.isZero())
        continue;
      
      ICELogInfo(<< "Checking interface " << ipInterface.toStdString());
      
      // Find local port number for this 
      int port = ipInterface.family() == AF_INET ? componentIter->second.mPort4 :
        componentIter->second.mPort6;

      // Define host candidate
      Candidate host(Candidate::Host);
      if ((ipInterface.family() == AF_INET && mConfig.mUseIPv4) ||
          (ipInterface.family() == AF_INET6 && mConfig.mUseIPv6))
      { 
        host.setLocalAndExternalAddresses(ipInterface, 
            ipInterface.family() == AF_INET6 ? componentIter->second.mPort6 : componentIter->second.mPort4);
        host.mComponentId = componentIter->first;
        host.mInterfacePriority = 0;
        // Gathering of host candidate is finished
        host.mReady = true; // Gathering finished
        host.mFailed = false; // With success

        mLocalCandidate.push_back(host);
      }

      // Check if IPv4 reflexive address is needed.
      // It can be gathered with IPv4 relaying request also
      bool needReflexiveAddr = mConfig.mUseIPv4 && mConfig.mServerList4.size() > 0;
      
      // Check if IPv4 relayed address is needed
      bool needRelayingAddr = 
        mConfig.mUseIPv4 && mConfig.mServerList4.size() > 0 && mConfig.mUseTURN;

      // Check if 6-to-4 relaying is required
      bool needIPv6ToIPv4 = !mConfig.mUseIPv4 && mConfig.mUseIPv6 &&
        !mConfig.mServerAddr6.isEmpty() && !mConfig.mServerAddr6.isZero() && mConfig.mUseProtocolRelay;

      // Check if 4-to-6 relaying is required
      /*bool needIPv4ToIPv6 = mConfig.mUseIPv4 && mConfig.mUseIPv6 &&
        mConfig.mServerList4.size() > 0 && mConfig.mUseProtocolRelay;
      */

      // Prepare candidate instances.
      Candidate reflexive(Candidate::ServerReflexive);
      Candidate relayed(Candidate::ServerRelayed);
                
      // Save initial local&external IP and port number
      relayed.setLocalAndExternalAddresses( ipInterface, port );
      reflexive.setLocalAndExternalAddresses( ipInterface, port );
        
      // Assign component ID
      relayed.mComponentId = reflexive.mComponentId = componentIter->first;

      // Put interface priority as zero - there is no configuration option for it yet
      relayed.mInterfacePriority = reflexive.mInterfacePriority = 0;
        
      bool restOnPublic = false;
#ifdef ICE_REST_ON_PUBLICIP
        if ( ipInterface.isPublic() && ipInterface.type() == AF_INET )
          restOnPublic = true;
#endif

      // See if there we need reflexive candidate yet
      if (mConfig.mUseSTUN && !restOnPublic && !bindingAction && 
          needReflexiveAddr && ipInterface.family() == AF_INET)
      {
        ICELogInfo(<< "Request reflexive address");
        // Add candidate to list
        mLocalCandidate.push_back(reflexive);
          
        // Bind transaction to candidate
        if (!bindingAction)
          bindingAction = new BindingAction(*this, mConfig);
      }

      if (mConfig.mUseTURN && !restOnPublic && !relayingAction && needRelayingAddr)
      {
        ICELogInfo(<< "Request relayed address");
        // Add relayed candidate to list
        mLocalCandidate.push_back(relayed);
        
        // Add reflexive candidate to list
        mLocalCandidate.push_back(reflexive);

        // Bind allocation to reflexive candidate 
        if (!relayingAction)
          relayingAction = new RelayingAction(*this, mConfig);
      }

      if (!getIp4Action && needIPv6ToIPv4 && ipInterface.family() == AF_INET6)
      {
        ICELogInfo(<< "Requesting IPv6 to IPv4 relay.");
        Candidate c(Candidate::ServerRelayed);
        c.setLocalAndExternalAddresses(ipInterface, port);
        c.mInterfacePriority = 0;
        c.mComponentId = componentIter->first;
        mLocalCandidate.push_back(relayed);
        getIp4Action = new RelayingAction(*this, mConfig);
      }
      
      /*
      if (!getIp6Action && needIPv4ToIPv6 && ipInterface.type() == AF_INET)
      {
        ICELogInfo(<< "Requesting IPv4 to IPv6 relay.");
        Candidate c(Candidate::ServerRelayed);
        c.setLocalAndExternalAddresses(ipInterface, port);
        c.mInterfacePriority = 0;
        c.mComponentId = componentIter->first;
        mLocalCandidate.push_back(relayed);
        getIp6Action = new RelayingAction(*this, mConfig);
      }
      */
    }

    if (bindingAction)
    {
      bind(mConfig.mServerAddr4, PAction(bindingAction), false/*it is gathering!*/, componentIter->first, mConfig.mServerList4.empty() ? FailoverOff : FailoverOn);
      numberOfActions++;
    }
    if (relayingAction)
    {
      ICELogDebug(<< "Request relayed candidate IPv4 <-> IPv4.");
      AllocateOptions options;
      options.mActionOnFinish = PAction(relayingAction);
      options.mComponent = componentIter->first;
      options.mFailoverOption = mConfig.mServerList4.empty() ? FailoverOff : FailoverOn;
      options.mServerAddress = mConfig.mServerAddr4;
      allocate(options);
      numberOfActions++;
    }
    
    if (getIp4Action)
    {
      ICELogDebug(<< "Request relayed candidate IPv6 <-> IPv4.");
      AllocateOptions options;
      options.mActionOnFinish = PAction(getIp4Action);
      options.mComponent = componentIter->first;
      options.mFailoverOption = mConfig.mServerList6.empty() ? FailoverOff : FailoverOn;
      options.mServerAddress = mConfig.mServerAddr6;
      options.mWireFamily = AF_INET6;
      options.mAllocFamily = AF_INET;
      // Ask TURN server to allocation IPv4 address via IPv6 address
      allocate(options);
      numberOfActions++;
    }
    
    // SDK does not use IPv4 to IPv6 relaying for now. Reverse direction has to be used instead.
    /*
    if (getIp6Action)
    {
      AllocateOptions options;
      options.mActionOnFinish = PAction(getIp6Action);
      options.mComponent = componentIter->first;
      options.mAllocFamily = AF_INET6;
      options.mWireFamily = AF_INET;
      options.mServerAddress = mConfig.mServerAddr4;
      options.mFailoverOption = mConfig.mServerList4.empty() ? FailoverOff : FailoverOn;
      // Ask TURN server to allocate IPv6 address via IPv4 socket
      allocate(options);
      numberOfActions++;
    }*/
    
  }
  
  if (!numberOfActions)
  {
    mState = EliminateRedudand;
    processStateChain();
  }
}

CheckList& Stream::checkList()
{
  return mCheckList;
}


void Stream::allocate(const AllocateOptions& options)
{
  assert(options.mComponent != -1);
  
  if (options.mFailoverOption == FailoverOn)
  {
    // If failover scheme is required - spread requests to few servers
    std::vector<NetworkAddress>& servers = options.mWireFamily == AF_INET ? mConfig.mServerList4 : mConfig.mServerList6;
    int failoverId = ++mFailoverIdGenerator;
    for (std::vector<NetworkAddress>::iterator addrIter = servers.begin(); addrIter != servers.end(); addrIter++)
    {
      AllocateOptions options2(options);
      options2.mServerAddress = *addrIter;
      options2.mFailoverId = failoverId;
      options2.mFailoverOption = FailoverOff;
      allocate(options2);
    }
  }
  else
  {
    ClientAllocate* allocation = new ClientAllocate(mConfig.mTurnLifetime);
    allocation->setStackId(mStackId);
    allocation->setWireFamily(options.mWireFamily);
    allocation->setAllocFamily(options.mAllocFamily);
    allocation->setDestination(options.mServerAddress);
    allocation->setComponent(options.mComponent);
    allocation->setFailoverId(options.mFailoverId);
    
    // Use auth for TURN server
    allocation->setPassword(mConfig.mTurnPassword);
    allocation->setUsername(mConfig.mTurnUsername);

    // Associate it with action
    allocation->setAction(options.mActionOnFinish);
    
    mActiveChecks.addRegular(allocation);
  }
}

void Stream::bind(const NetworkAddress& dest, PAction action, bool /*auth*/, int component, Failover f)
{
  assert(component != -1);
  
  if (f == FailoverOn)
  {
    for (std::vector<NetworkAddress>::iterator addrIter = mConfig.mServerList4.begin(); addrIter != mConfig.mServerList4.end(); addrIter++)
    {
      bind(*addrIter, action, false, component, FailoverOff);
    }
  }
  else
  {
    ClientBinding* binding = new ClientBinding();
    binding->setStackId(mStackId);
    binding->setDestination(dest);
    binding->setComponent(component);

    // Associate it with action
    binding->setAction(action);

    mActiveChecks.addRegular(binding);
  }
}


bool Stream::hasPortNumber(int family, unsigned short portNumber, int* component)
{
  ComponentMap::iterator componentIterator;
  for (componentIterator = mComponentMap.begin(); componentIterator != mComponentMap.end(); ++componentIterator)
  {
    if ((family == AF_INET && componentIterator->second.mPort4 == portNumber) ||
        (family == AF_INET6 && componentIterator->second.mPort6 == portNumber))
    {
      if (component)
        *component = componentIterator->first;
      return true;
     }
  }
  
  return false;
}

unsigned Stream::findRemoteCandidate(const NetworkAddress& addr,  int componentID)
{
  for (size_t i=0; i<mRemoteCandidate.size(); i++)
  {
    Candidate& cand = mRemoteCandidate[i];
    
    if (cand.mExternalAddr == addr && cand.mComponentId == componentID)
      return (unsigned)i;
  }

  return NoIndex;
}

unsigned Stream::findLocalCandidate(const NetworkAddress& addr)
{
  for (size_t i=0; i<mLocalCandidate.size(); i++)
  {
    Candidate& cand = mLocalCandidate[i];
    
    if (cand.mExternalAddr == addr)
      return (unsigned)i;
  }

  return NoIndex;
}

void Stream::cancelCheck(CandidatePair& p)
{
  if (mActiveChecks.exists(reinterpret_cast<Transaction*>(p.transaction())))
  {
    ICELogInfo(<< "Transaction for pair " << p.toStdString() << " is cancelled");
    mActiveChecks.erase((Transaction*)p.transaction());
  }
}

class EraseBindingAndRelaying: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    ClientRefresh* c = dynamic_cast<ClientRefresh*>(t);
    if (c)
    {
      if (!c->lifetime())
        return false;
    }
    
    if ((Transaction::Binding | Transaction::Relaying) & t->type())
      return true;
    
    return false;
  }
};

class KeepDeallocationRequest: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    ClientRefresh* c = dynamic_cast<ClientRefresh*>(t);
    if (c)
    {
      if (!c->lifetime())
        return false;
    }
    return true;
  }
};


void Stream::cancelAllocations()
{
  mActiveChecks.erase(KeepDeallocationRequest());
}

void Stream::handleBindingRequest(ServerBinding& binding, ByteBuffer& buffer, int component)
{
  if (mComponentMap.find(component) == mComponentMap.end())
    return;

  // If the source transport address of the request does not match any
  // existing remote candidates, it represents a new peer reflexive remote
  // candidate.  This candidate is constructed as follows:
  unsigned candidateIndex = findRemoteCandidate(buffer.remoteAddress(), component);
  if (candidateIndex == NoIndex)
  {
    ICELogInfo(<< "Stack ID " << mStackId << ". Remote candidate with address " << buffer.remoteAddress().toStdString() << " is not found so creating peer reflexive candidate.");

    // Construct candidate
    Candidate newRemoteCand(Candidate::PeerReflexive);
    
    // Set priority from request's priority attribute
    newRemoteCand.mPriority = binding.priorityValue();

    // Set address
    newRemoteCand.setLocalAndExternalAddresses(buffer.remoteAddress());
    
    // The foundation of the candidate is set to an arbitrary value,
    // different from the foundation for all other remote candidates.  If
    // any subsequent offer/answer exchanges contain this peer reflexive
    // candidate in the SDP, it will signal the actual foundation for the
    // candidate.
    sprintf(newRemoteCand.mFoundation, "%u", --mFoundationGenerator);
    
    // The component ID of this candidate is set to the component ID for
    // the local candidate to which the request was sent.
    newRemoteCand.mComponentId = component;
    
    // This candidate is added to the list of remote candidates.  However,
    // the agent does not pair this candidate with any local candidates.
    mRemoteCandidate.push_back(newRemoteCand);
    
    candidateIndex = (unsigned)mRemoteCandidate.size()-1;
  }

  // Next, the agent constructs a pair whose local candidate is equal to
  // the transport address on which the STUN request was received, and a
  // remote candidate equal to the source transport address where the
  // request came from (which may be peer-reflexive remote candidate that
  // was just learned).  
  CandidatePair _pair;
  
  // Find local transport address (IP)
  NetworkAddress interfaceIP;
  if (buffer.relayed())
  {
    // Find relayed ip:port for given component
    for (unsigned i=0; i<mLocalCandidate.size(); i++)
      if (mLocalCandidate[i].mType == Candidate::ServerRelayed && mLocalCandidate[i].component() == buffer.component())
        interfaceIP = mLocalCandidate[i].mExternalAddr;
  }
  
  // If looking for relayed candidate failed or it is not relayed candidate at all - just find source interface for peer address
  if (interfaceIP.isEmpty())
  {
    interfaceIP = NetworkHelper::instance().sourceInterface(buffer.remoteAddress());
  
    // Find local port number
    switch (buffer.remoteAddress().family())
    {
      case AF_INET:
        interfaceIP.setPort(mComponentMap[buffer.component()].mPort4);
        break;

      case AF_INET6:
        interfaceIP.setPort(mComponentMap[buffer.component()].mPort6);
        break;
    }
  }

  // And try to find local candidate with such address - addrToFind
  unsigned existingLocalCandIndex = findLocalCandidate(interfaceIP);
  
  // There MUST be such candidate!
  if (existingLocalCandIndex == NoIndex)
  {
    unsigned short port = interfaceIP.port();
    std::string ip = interfaceIP.ip();
    ICELogCritical(<<"Failed to find local candidate for " << ip << ":" << port << ". Ignoring binding request");
    return;
  }
	
  // Construct pair
  _pair.first() = mLocalCandidate[existingLocalCandIndex];
  _pair.second() = mRemoteCandidate[candidateIndex];

  // Since both candidates are known to the agent, it
  // can obtain their priorities and compute the candidate pair priority.
  _pair.updateFoundation();
  _pair.updatePriority();
  
  ICELogInfo(<< "Stack ID " << mStackId << ". Constructed candidate pair " << _pair.toStdString());

  if (mAgentRole == RoleControlled && binding.hasUseCandidate())
  {
    _pair.setNomination(CandidatePair::Nomination_Finished);
    ICELogInfo(<< "Stack ID " << mStackId << ". Pair " << _pair.toStdString() << " nominated as received incoming Binding request with UseCandidate attribute.");
  }

  // This pair is then looked up in the check list.  
  PCandidatePair existingPair = mCheckList.findEqualPair(_pair, CheckList::CT_TreatHostAsUniform);
  
  if (existingPair)
  {
    // Nominate new pair if the similar pair was nominated before
    if (_pair.nomination() == CandidatePair::Nomination_Finished)
      existingPair->setNomination(CandidatePair::Nomination_Finished);

    ICELogInfo(<< "Stack ID " << mStackId << ". Pair " << existingPair->toStdString()
							 << " already exists in check list. Its state is " 
							 << CandidatePair::stateToString(existingPair->state()));

	if (mConfig.mTreatRequestAsConfirmation)
    {
      if (existingPair->transaction())
      {
        ConnectivityCheck* cc = dynamic_cast<ConnectivityCheck*>((Transaction*)existingPair->transaction());
        if (cc)
        {
          if (!cc->removed())
            cc->confirmTransaction(buffer.remoteAddress());
        }
      }
    }

    // There can be one of several outcomes:
    // If the state of that pair is Waiting or Frozen, a check for
    //    that pair is enqueued into the triggered check queue if not
    //    already present.
    switch (existingPair->state())
    {
    case CandidatePair::Frozen:
    case CandidatePair::Waiting:
      existingPair->setRole(CandidatePair::Triggered);
      existingPair->setState(CandidatePair::Waiting);
      break;

    case CandidatePair::InProgress:
      // If the state of that pair is In-Progress, the agent cancels the
      // in-progress transaction.  Cancellation means that the agent
      // will not retransmit the request, will not treat the lack of
      // response to be a failure, but will wait the duration of the
      // transaction timeout for a response.  In addition, the agent
      // MUST create a new connectivity check for that pair
      // (representing a new STUN Binding Request transaction) by
      // enqueuing the pair in the triggered check queue.  The state of
      // the pair is then changed to Waiting.
      if (existingPair->role() == CandidatePair::Regular)
      { 
        cancelCheck(*existingPair);
        existingPair->setState(CandidatePair::Waiting);
        existingPair->setRole(CandidatePair::Triggered);
        
        // Re-run new check in prioritized list
        Transaction* t = createCheckRequest(existingPair);
        if (buffer.relayed())
        {
          t->setComment("Check from " + existingPair->first().mExternalAddr.toStdString() +
                        " to " + existingPair->second().mExternalAddr.toStdString());
          t->setRelayed(true);
        }
        existingPair->setTransaction(t);
        mActiveChecks.addPrioritized(t);
      }
      break;
    
    case CandidatePair::Failed:
      // If the state of the pair is Failed, it is changed to Waiting
      // and the agent MUST create a new connectivity check for that
      // pair (representing a new STUN Binding Request transaction), by
      // enqueuing the pair in the triggered check queue.
      existingPair->setState(CandidatePair::Waiting);
      existingPair->setRole(CandidatePair::Triggered);
      break;

    case CandidatePair::Succeeded:
      //existingPair.setNominated();
      break;
    }
  }
  else
  {
    // If the pair is not already on the check list:
    // The pair is inserted into the check list based on its priority

    // Its state is set to Waiting
    _pair.setState(CandidatePair::Waiting);
    _pair.setRole(CandidatePair::Triggered);
    ICELogInfo(<< "Stack ID " << mStackId << ". Adding pair " << _pair.toStdString() << " to triggered check list");
    mCheckList.add(_pair);
    ICELogInfo(<< "Resulting check list is \r\n" << mCheckList.toStdString());
  }
}

bool Stream::processData(StunMessage& msg, ByteBuffer& buffer, int component)
{
  bool result = true;
  
  // Depending on the state call the handler
  if (mState < CreatingSDP)
  {
    result = handleCgIn(msg, buffer.remoteAddress());
    if (result)
      processStateChain();
    return result;
  }
  else
  if (mState >= ConnCheck)
  {
    if (handleConnChecksIn(msg, buffer.remoteAddress()))
    {
      ICELogDebug(<< "Found response for transaction from " << buffer.remoteAddress().toStdString());
      checkNominated();
      return true;
    }
  }
  else
  if (handleConnChecksIn(msg, buffer.remoteAddress()))
    return true;

  
  // Check if source address&request belongs to TURN server - so it is relayed candidate in action
  if (buffer.remoteAddress() == mConfig.mServerAddr4)
  {
    if (buffer.size() < 4)
      return false;

    TurnPrefix prefix = *reinterpret_cast<const TurnPrefix*>(buffer.data());
      
    // For requests being received on a relayed candidate, the source
    // transport address used for STUN processing (namely, generation of the
    // XOR-MAPPED-ADDRESS attribute) is the transport address as seen by the
    // TURN server. That source transport address will be present in the
    // REMOTE-ADDRESS attribute of a Data Indication message, if the Binding
    // Request was delivered through a Data Indication (a TURN server
    // delivers packets encapsulated in a Data Indication when no active
    // destination is set).  If the Binding Request was not encapsulated in
    // a Data Indication, that source address is equal to the current active
    // destination for the TURN session.

    //check channel binding prefix
    if (mTurnPrefixMap.find(prefix) != mTurnPrefixMap.end())
    {
      NetworkAddress& sourceAddr = mTurnPrefixMap[prefix];
      buffer.setRemoteAddress(sourceAddr);
      buffer.erase(0, 4);
      buffer.setRelayed(true);
    }
  }
    
  handleIncomingRequest(msg, buffer, component);
        
  if (mCheckList.state() == CheckList::Failed)
    mState = Failed;
  else
   checkNominated();
    
  return result;
}


bool Stream::handleCgIn(StunMessage& msg, NetworkAddress& address)
{
  if (!mActiveChecks.processIncoming(msg, address))
    return false;
  if (mLocalCandidate.empty())
    return true;
  
  // Check if failover and relaying is used
  if (mConfig.mUseTURN && !mConfig.mServerList4.empty())
  {
    int successCounter = 0, failedCounter = 0;
    for (size_t i=0; i<mLocalCandidate.size(); i++)
      if (mLocalCandidate[i].mReady)
      {
        if (mLocalCandidate[i].mFailed)
          failedCounter++;
        else
          successCounter++;
      }
    ICELogDebug(<< "Succeed " << successCounter << " candidates, failed " << failedCounter << " candidates from " << (int)mLocalCandidate.size());
    if ((size_t)successCounter >= mLocalCandidate.size())
    {
      // Shift to next state
      ICELogInfo(<< "All candidates succeeded, shift to EliminateRedudand");
      mState = EliminateRedudand;
    }
    else
    {
      // See if all gathering requests are performed.
      if ((size_t)mFailoverRequestsFinished == mConfig.mServerList4.size() * mComponentMap.size())
      {
        ICELogInfo(<< "All failover gathering requests finished, shift to Failed");
        mState = Failed;
      }
    }
  }
  else
  {
    // Look if all gathering checks are finished
    bool finished = true;
  
    for (size_t i=0; i<mLocalCandidate.size() && finished; i++)
      if (false == mLocalCandidate[i].mReady)
        finished &= false;
  
    // NOTE! Checking for timeout is made in other place!
    if (finished && mState != /*RunningState::*/Failed)
      mState = EliminateRedudand;
  }
  return true;
}


bool FailedCand ( const ice::Candidate& value ) {
  return value.mFailed;
}

bool GreaterCand( const ice::Candidate& c1, const ice::Candidate& c2)
{
  if (c1.mExternalAddr.port() > c2.mExternalAddr.port())
    return true;
  else
  if (c1.mExternalAddr.port() < c2.mExternalAddr.port())
    return false;

  if (c1.mExternalAddr.ip() > c2.mExternalAddr.ip())
    return true;
  else
    return false;
}

bool RedudandCand( const ice::Candidate& c1, const ice::Candidate& c2)
{
  return c1.mExternalAddr == c2.mExternalAddr;
}

void Stream::Handle_ER()
{
  // Remove failed candidates (without STUN/TURN responses)
  mLocalCandidate.erase(std::remove_if(mLocalCandidate.begin(), mLocalCandidate.end(), 
    FailedCand), mLocalCandidate.end());

  // Sort candidates by priority
  unsigned int cand_per_component = unsigned(( mLocalCandidate.size() ) / mComponentMap.size());
  for (unsigned int i=0; i<mComponentMap.size(); i++)
  {
    CandidateVector::iterator bit = mLocalCandidate.begin() + i * cand_per_component;
    CandidateVector::iterator fit = mLocalCandidate.begin() + (i+1) * cand_per_component;
    if (i == mComponentMap.size() - 1)
      fit = mLocalCandidate.end();

    std::sort(bit, fit, GreaterCand);
  }

  // Leave only unique candidates
  mLocalCandidate.erase(std::unique(mLocalCandidate.begin(), mLocalCandidate.end(), RedudandCand), mLocalCandidate.end());
  
  mState = ComputingFoundations;
}

void Stream::Handle_CF()
{
  //create single list of all candidates
  CandidateVector::iterator cit;

  for (cit = mLocalCandidate.begin(); cit != mLocalCandidate.end(); ++cit)
    cit->computeFoundation();

  mState = StartingKeepAlives;
}

void Stream::Handle_SKA()
{
#ifdef ICE_ENABLE_KEEPALIVE
    //TODO: replace successful transactions to keep alive state
#endif
  
  mState = PrioritizingCandidates;
}

bool PriorityGreater(const Candidate& c1, const Candidate& c2)
{
  return c1.mPriority > c2.mPriority;
}

void Stream::Handle_PC()
{
  for (size_t i = 0; i < mLocalCandidate.size(); ++i)
    mLocalCandidate[i].computePriority(mConfig.mTypePreferenceList);
  
  // Find the candidates range for each component ID
  ComponentMap::iterator componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
  {
    // Find range start
    size_t minIndex = 0xFFFFFFFF, maxIndex = 0;
    
    for (CandidateVector::size_type i = 0; i < mLocalCandidate.size(); i++)
    {
      if (mLocalCandidate[i].mComponentId == (int)componentIter->first)
      {
        // Wow, we have found first candidate with the specified componentID
        minIndex = i;

        break;
      }
    }

    // Find range max
    for (int i = (int)mLocalCandidate.size()-1; i >=0; i--)
    {
      if (mLocalCandidate[i].mComponentId == componentIter->first)
      {
        // Wow, we have found first candidate with the specified componentID
        maxIndex = i;

        break;
      }
    }
    
    if (minIndex == 0xFFFFFFFF)
      return;

    CandidateVector::iterator bit = mLocalCandidate.begin() + minIndex;
    CandidateVector::iterator fit;
    if (maxIndex == mLocalCandidate.size()-1)
      fit = mLocalCandidate.end();
    else
      fit = mLocalCandidate.begin() + maxIndex + 1;

    std::sort(bit, fit, PriorityGreater);
  }
  
  mState = ChoosingDefault;
}

void Stream::Handle_CD()
{
  mDefaultCandidate.clear();
  ComponentMap::iterator componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
    mDefaultCandidate[componentIter->first] = findDefaultCandidate(componentIter->first);
  
  if (mDefaultCandidate.empty())
    throw std::logic_error("Cannot find default candidate.");

  mState = CreatingSDP;
}

Candidate Stream::findDefaultCandidate(int componentID)
{
  // Extract all candidates for specified component to separate vector
  CandidateVector candidateList;
  for (size_t i=0; i<mLocalCandidate.size();i++)
    if (mLocalCandidate[i].mComponentId == componentID)
      candidateList.push_back(mLocalCandidate[i]);
  
  // Check if we have reflexive candidate - return it
  for (size_t i=0; i<candidateList.size(); i++)
    if (candidateList[i].mType == Candidate::ServerReflexive)
      return candidateList[i];

  // Find best interface for public IP peer address
  NetworkAddress foundIP = NetworkHelper::instance().sourceInterface(NetworkAddress(mConfig.mTargetIP, 1000));
  if (!foundIP.isEmpty())
  {
    for (size_t i=0; i<candidateList.size(); i++)
    {
      if (candidateList[i].mType == Candidate::Host && 
          candidateList[i].mLocalAddr.ip() == foundIP.ip())
        return candidateList[i];
    }
  }
  
  for (size_t i=0; i<candidateList.size(); i++)
    if (candidateList[i].mType == Candidate::Host)
      return candidateList[i];
  
  return Candidate(Candidate::Host);
}

bool Stream::handleConnChecksIn(StunMessage& msg, NetworkAddress& address)
{
/*  if (mRemotePwd.empty())
    return false; */

  bool result;
  result = mActiveChecks.processIncoming(msg, address);  
  return result;
}

void Stream::handleIncomingRequest(StunMessage& msg, ByteBuffer& buffer, int component)
{ 
  // Check if it is binding request
  ServerBinding binding;
    
  // Use Fingerprint&MessageIntegrity attributes to generate responses
  binding.addFingerprint(true);
  binding.addShortTermCredentials(true);
    
  // Set username and password
  binding.setUsername(mLocalUfrag + ":" + mRemoteUfrag);
  binding.setPassword(mLocalPwd);
  
  // Is this request Bind?
  if (binding.processData(msg, buffer.remoteAddress()))
  {
    // Check if packet has message integrity with local password
    if (!msg.validatePacket(mLocalPwd))
      return;

    // Is binding processed ok?
    if (binding.gotRequest())
    {
      // Is role conflict detected? it happens on user agent level - not streams or components
      if (binding.role() == mAgentRole && binding.role())
      {
        bool errorGenerated = handleRoleConflict(binding);
        if (errorGenerated)
          return;
      }
      else
      {
        ICELogInfo( << "Stack ID " << mStackId << ". Got a binding request from " << buffer.remoteAddress().toStdString() << ".");
        
        handleBindingRequest(binding, buffer, component);
      } //of Generate 400
      
      // Create response packet
      ByteBuffer* responseBuffer = binding.generateData(true);
      
      // If there is a response packet
      if (responseBuffer)
      {
        // Set comment
        responseBuffer->setComment("Response for connectivity check request.");
          
        // Set destination address
        // If request has come via relayed candidate - send it back using SendIndication
        if (buffer.relayed())
        {
          ByteBuffer* si = SendIndication::buildPacket( buffer.remoteAddress(), *responseBuffer, mConfig.mServerAddr4, buffer.component() );
          delete responseBuffer; responseBuffer = si;
          responseBuffer->setComment( "SI for CC response to " + buffer.remoteAddress().toStdString() );
        }
        else
          responseBuffer->setRemoteAddress( buffer.remoteAddress() );
        
        // Set component port number
        responseBuffer->setComponent(buffer.component());

        // Send response immediately
        mResponseQueue.push_back(responseBuffer);
      }
    }
  }
}

void Stream::checkNominated(int component)
{
  // If there are no nominated pairs in the valid list for a media
  // stream and the state of the check list is Running, ICE processing
  // continues.
  if (mCheckList.state() != CheckList::Running)
    return;

  // Attempt to find nominated pair
  unsigned i;
  for (i=0; i<mCheckList.count() && (mCheckList[i]->nomination() != CandidatePair::Nomination_Finished || mCheckList[i]->state() != CandidatePair::Succeeded); i++)
    ;
  
  if (i == mCheckList.count())
    return;

  // If there is at least one nominated pair in the valid list for a
  //  media stream and the state of the check list is Running:

  // The agent MUST remove all Waiting and Frozen pairs in the check
  //  list and triggered check queue for the same component as the
  //  nominated pairs for that media stream
  mCheckList.removePairs(CandidatePair::Frozen, component);
  mCheckList.removePairs(CandidatePair::Waiting, component);
    
#ifndef ICE_DONT_CANCEL_CHECKS_ON_SUCCESS
  //  If an In-Progress pair in the check list is for the same
  //  component as a nominated pair, the agent SHOULD cease
  //  retransmissions for its check if its pair priority is lower
  //  than the lowest priority nominated pair for that component.
  PCandidatePair lowestPair = mCheckList.findLowestNominatedPair(component);
  if (lowestPair)
  {
    int64_t lowestPriority = lowestPair->priority();

    for (unsigned i=0; i<mCheckList.count(); i++)
    {
      CandidatePair& pair = *mCheckList[i];
      if (((pair.state() == CandidatePair::InProgress && 
          pair.priority() < lowestPriority) || pair.nomination() != CandidatePair::Nomination_Finished) && pair.first().component() == component)
        cancelCheck(pair);
    }
  }
#endif
}

void Stream::checkNominated()
{
  if (mState == Success)
    return;
  
  // Check if there is running nomination waiting interval timer
  if (ICE_NOMINATION_WAIT_INTERVAL)
  {
    for (auto componentIter = mComponentMap.begin(); componentIter != mComponentMap.end(); ++componentIter)
    {
      if (componentIter->second.mNominationWaitIntervalStartTime && mAgentRole == RoleControlling)
      {
        PCandidatePair validPair = mCheckList.findBestValidPair(componentIter->first);
        if (validPair)
        {
          unsigned currentTime = ICETimeHelper::timestamp();
          if (ICETimeHelper::findDelta(componentIter->second.mNominationWaitIntervalStartTime, currentTime) >= ICE_NOMINATION_WAIT_INTERVAL)
            nominatePair(validPair);
        }
      }
    }
  }
  
  if (mNominationWaitStartTime && mCheckList.countOfValidPairs() && mAgentRole == RoleControlling)
  {
    unsigned currentTime = ICETimeHelper::timestamp();
    if (ICETimeHelper::findDelta(mNominationWaitStartTime, currentTime) >= ICE_NOMINATION_WAIT_INTERVAL)
    {
      // Iterate components
      for (auto componentIter = mComponentMap.begin(); componentIter != mComponentMap.end(); ++componentIter)
      {
        PCandidatePair validPair = mCheckList.findBestValidPair(componentIter->first);
        if (validPair)
          nominatePair(validPair);
      }
    }
  }

  // Process all components
  ComponentMap::iterator componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
    checkNominated(componentIter->first);

  // Once there is at least one nominated pair in the valid list for
  //  every component of at least one media stream and the state of the
  //  check list is Running:

  // The agent MUST change the state of processing for its check
  //  list for that media stream to Completed.

  bool found = true;
  componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
    found &= mCheckList.findNominatedPair(componentIter->first).get() != nullptr;

  if (found && mCheckList.state() == CheckList::Running)
  {
    mCheckList.setState(CheckList::Completed);
    
    // Create vector of default 
    std::map<int, Candidate> defaultCandList;
    componentIter = mComponentMap.begin();
    for (; componentIter != mComponentMap.end(); componentIter++)
    {
      int componentId = componentIter->first;
      PCandidatePair highestPair = mCheckList.findHighestNominatedPair(componentId);
      if (highestPair)
      {
        defaultCandList[componentId] = Candidate(highestPair->first());
      
        // Compare with existing default IP list
        if (!Candidate::equal(defaultCandList[componentId], mDefaultCandidate[componentId]))
          mDefaultIPChanged = true;
      }
    }
    
    mDefaultCandidate = defaultCandList;
    mCanTransmit = true;
  }
  
  // Once the state of each check list is Completed:
  //ICELogDebug(<< "Stack ID " << mStackID << ". Check list state is " << mCheckList.stateToString(mCheckList.state()));
  if (mCheckList.state() == CheckList::Completed)
  {
    // The agent sets the state of ICE processing overall to
    // completed.
    if (mState != Success && mState > CandidateGathering)
    {
      ICELogCritical( << "Stack ID " << mStackId << ". Check list is completed, so session is succeeded.");
      mState = Success;
    }
  }
  else
  if (mCheckList.state() == CheckList::Failed)
  {
    //  If the state of the check list is Failed, ICE has not been able to
    //  complete for this media stream.  The correct behavior depends on
    //  the state of the check lists for other media streams:
    
    //  If all check lists are Failed, ICE processing overall is
    //  considered to be in the Failed state, and the agent SHOULD
    //  consider the session a failure, SHOULD NOT restart ICE, and the
    //  controlling agent SHOULD terminate the entire session.
    if (mState != Failed && mState > CandidateGathering)
    {
      ICELogCritical(<< "Stack ID " << mStackId << ". Check list is failed, so session is failed too.");
      mState = Failed;
    }
    clearChecks();
    //TODO:
    //If at least one of the check lists for other media streams is
    //Completed, the controlling agent SHOULD remove the failed media
    //stream from the session in its updated offer.

    //If none of the check lists for other media streams are
    //Completed, but at least one is Running, the agent SHOULD let
    //ICE continue.
  }
}

bool Stream::handleRoleConflict(ServerBinding& binding)
{
  ICELogCritical( << "Stack ID " << mStackId << ". Detected role conflict. Local role is " << 
    (mAgentRole == RoleControlled ? "Controlled" : "Controlling") << ", remote role is " <<
    (binding.role() == RoleControlled ? "Controlled" : "Controlling") << ".");
  
  // Compare tie breakers
  int compareResult = memcmp(mTieBreaker.c_str(), binding.remoteTieBreaker().c_str(), 8);

  if (mAgentRole == RoleControlling)
  {
    // So remove 
    if (compareResult >= 0)
    {
      // Queue 487 error to remote peer
      binding.generate487();
      return true;
    }
    else
      mAgentRole = RoleControlled;
  }
  else
  {
    if (compareResult < 0)
    {
      // Enqueue 487 error to remote peer
      binding.generate487();
      return true;
    }
    else
      mAgentRole = RoleControlling;
  }

  // Recompute pair priorities for all stream's queues and triggered check queue
  mCheckList.updatePairPriorities();

  return false;
}

void Stream::createOfferSdp(std::vector<std::string>& defaultIP, std::vector<unsigned short>& defaultPort, 
                                 std::vector<std::string>& candidateList)
{
  // Iterator all components
  ComponentMap::iterator componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
  {
    // Find default IP for this component
    defaultIP.push_back(mDefaultCandidate[componentIter->first].mExternalAddr.ip());
    defaultPort.push_back(mDefaultCandidate[componentIter->first].mExternalAddr.port());

    for (unsigned candidateCounter=0; candidateCounter<mLocalCandidate.size(); candidateCounter++)
    {
      Candidate& cand = mLocalCandidate[candidateCounter];
    
      if (cand.mComponentId == (int)componentIter->first)
      {
        candidateList.push_back(cand.createSdp());
#ifdef ICE_VIRTUAL_CANDIDATES
        if (cand.mType == Candidate::ServerReflexive || cand.mType == Candidate::PeerReflexive)
        {
          for (int i=0; i<ICE_VIRTUAL_CANDIDATES; i++)
          {
            Candidate v = cand;
            v.mExternalAddr.setPort(v.mExternalAddr.port()+i+1);
            candidateList.push_back(v.createSdp());
          }
        }
#endif
      }
    }
  }
}

NetworkAddress Stream::defaultAddress(int componentID)
{
  if ((int)mDefaultCandidate.size() >= componentID)
    return mDefaultCandidate[componentID].mExternalAddr;

  if (mComponentMap.find(componentID) != mComponentMap.end())
  {
    NetworkAddress result = NetworkHelper::instance().sourceInterface(NetworkAddress(mConfig.mTargetIP, 1));
    result.setPort(mComponentMap[componentID].mPort4);
    return result;
  }
  return NetworkAddress();
}

void Stream::candidateList(int componentID, std::vector<std::string>& candidateList)
{
  for (unsigned i=0; i<mLocalCandidate.size(); i++)
  {
    Candidate& cand = mLocalCandidate[i];
    
    if ((cand.mComponentId == componentID || componentID == -1) && cand.mReady && !cand.mFailed)
    {
      candidateList.push_back(cand.createSdp());
#ifdef ICE_VIRTUAL_CANDIDATES
      if (cand.mType == Candidate::ServerReflexive || cand.mType == Candidate::PeerReflexive)
      {
        for (int i=0; i<ICE_VIRTUAL_CANDIDATES; i++)
        {
          Candidate v = cand;
          v.mExternalAddr.setPort(v.mExternalAddr.port()+i+1);
          candidateList.push_back(v.createSdp());
        }
      }
#endif
    }
  }
}

void Stream::isTimeout()
{
  mActiveChecks.checkTimeout();
}

PByteBuffer Stream::getDataToSend(bool& response, int& component, void*&tag)
{
  // All responses are served at half priority
  PByteBuffer result;
  if (!mResponseQueue.empty())
  {
    //ICELogInfo(<<"Return to send response packet");
    result = PByteBuffer(mResponseQueue.front());
    mResponseQueue.erase(mResponseQueue.begin());
    response = true;
  }
  else
  {
    response = false;
    result = PByteBuffer(handleConnChecksOut());       // Generates connectivity check/binding requests/responses
    if (result)
    {
      //ICELogInfo(<<"Return to send request packet");
    }
  }

  if (!result)
    return PByteBuffer();
  
  component = result->component();
  tag = mComponentMap[component].mTag;
  
  return result;
}

Transaction* Stream::runCheckList(CandidatePair::Role r, CandidatePair::State state)
{
  Transaction* result = NULL;
  for (unsigned i=0; i<mCheckList.count() && !result; i++)
  {
    // Extract reference to candidate pair
    PCandidatePair& p = mCheckList[i];

    if (p->role() != r || p->transaction())
      continue;

    if (p->state() == state)
    {
      ICELogInfo(<<"Creating check request for " << p->toStdString());

      // Create binding request
      result = createCheckRequest(p);
      
      // Mark candidate pair as InProgress
      p->setTransaction(result);
      p->setState(CandidatePair::InProgress);
    }
  }
  return result;
}

ByteBuffer* Stream::handleConnChecksOut()
{
  // Update stream state
  checkNominated();
  
  // Loop while we have time to send packets
  if (mScheduleTimer.isTimeToSend())
  {
    // Declare pointer to created check request
    Transaction* request = NULL;
    
    // Check for triggered request
    //ICELogInfo(<<"Create checks for triggered waiting pairs for list " << std::endl << mCheckList.toStdString());
    request = runCheckList(CandidatePair::Triggered, CandidatePair::Waiting);
    if (request)
      mActiveChecks.addPrioritized(request);
    else
    {
      request = runCheckList(CandidatePair::Regular, CandidatePair::Waiting);
      if (request)
        mActiveChecks.addRegular(request);
      else
      {
        // Find Frozen check with highest priority and lowest componentID in check list
        request = runCheckList(CandidatePair::Regular, CandidatePair::Frozen);
        if (request)
          mActiveChecks.addRegular(request);
      }
    }

  }
  
  // ICELogDebug(<< "Looking for transaction to send");
  // Loop while active transaction is not found
  Transaction* t; int attemptCounter = 0;
  do 
  {
    attemptCounter++;
    t = mActiveChecks.getNextTransaction();
    if (!t)
      continue;
    // This code forces keepalive transaction to its intervals
    if (!t->hasToRunNow())
      t = NULL;
  }
  while (!t && attemptCounter < mActiveChecks.count());
  
  if (!t)
    return NULL;
  
  ByteBuffer* buffer = t->generateData();
  if (!buffer)
    return NULL;

  // Self test
  /*StunMessage msg;
  msg.parsePacket(*buffer);
  std::ostringstream oss;
  msg.dump(oss);
  ICELogDebug(<< "Self test result: " << oss.str());
  */
  ICELogInfo(<< "Generating packet for " << (t->relayed() ? "relayed " : "") << t->comment() << " to " << t->destination().toStdString());
  if (t->relayed())
  {
    ByteBuffer* si = SendIndication::buildPacket(t->destination(), *buffer, mConfig.mServerAddr4, t->component());
    si->setComment("Relayed: " + buffer->comment());
    delete buffer; buffer = si;
  }

  assert(!buffer->remoteAddress().isEmpty());
  return buffer;
}


Transaction* Stream::createCheckRequest(PCandidatePair& p)
{
  // Get reference to checked pair

  // Create transaction - depending on candidates it can be regular ConnectivityCheck or RelayedConnectivityCheck
  ConnectivityCheck* cc = new ConnectivityCheck();

  cc->setRelayed( p->first().mType == Candidate::ServerRelayed );
  cc->setStackId( mStackId );
  cc->setComponent( p->first().mComponentId );

  // Add PRIORITY attribute
  unsigned int peerReflexivePriority = 0xFFFFFF * mConfig.mTypePreferenceList[Candidate::PeerReflexive] + 
      (p->first().mInterfacePriority << 8) + (256 - p->first().mComponentId);

  cc->addPriority( peerReflexivePriority );
  
  cc->addFingerprint( true );
  cc->setDestination( p->second().mExternalAddr );
  cc->setComment( p->toStdString() );
  
  // Add CONTROLLED or CONTROLLING attribute
  switch (mAgentRole)
  {
  case RoleControlled:
    cc->addControlledRole(mTieBreaker);
    break;

  case RoleControlling:
    cc->addControllingRole(mTieBreaker);
    break;
    
  default:
    assert(0);
  }
  
  // Enable short term credentials
  cc->addShortTermCredentials(true);
  
  // Set right username and password
  cc->setUsername(mRemoteUfrag + ":" + mLocalUfrag);
  cc->setPassword(mRemotePwd);
  
  // Add action
  cc->setAction(PAction(new CheckPairAction(*this, mConfig, p)));
  
  // Do not forget about aggressive nomination
  if (mConfig.mAggressiveNomination)
    cc->addUseCandidate();

  ICELogInfo( << "Stack ID " << mStackId << ". Created " << (cc->relayed() ? "RCC" : "CC") << " for " << p->toStdString() << " and destination " <<  cc->destination().toStdString() << ".");

  return cc;
}

TurnPrefix Stream::findTurnPrefixByAddress(NetworkAddress& peerAddress)
{
  TurnPrefixMap::iterator tpit = mTurnPrefixMap.begin();
  for (; tpit != mTurnPrefixMap.end(); ++tpit)
  {
    NetworkAddress& addr = tpit->second;
    if (addr == peerAddress)
      return tpit->first;
  }

  return 0;
}

class KeepAliveCheckCondition: public TransactionCondition
{
protected:
  NetworkAddress mTarget;
  int mComponentId;
  
public:
  KeepAliveCheckCondition(const NetworkAddress& address, int componentId)
  :mTarget(address), mComponentId(componentId)
  {
    
  }
  
  bool check(Transaction* t) const
  {
    BindingIndication* bi = dynamic_cast<BindingIndication*>(t);
    if (!bi)
      return false;
    if (!bi->keepalive())
      return false;
    
    return (t->component() == mComponentId && t->destination() == mTarget);
  }
};

void Stream::addKeepAliveCheck(CandidatePair& _pair)
{
#ifdef ICE_ENABLE_KEEPALIVE
  // Ensure it is new keepalive check
  if (mActiveChecks.exists(KeepAliveCheckCondition(_pair.second().mExternalAddr, _pair.first().mComponentId)))
    return;
  
  // Create binding indication transaction
  BindingIndication* bi = new BindingIndication(mConfig.mKeepAliveInterval);
  bi->setStackId(mStackId);
  bi->setKeepalive(true);
  bi->setInterval(mConfig.mKeepAliveInterval);
  
  // Restart transaction on finish
  //bi->setUserObject(new CandidatePair(_pair));

  // Do not forget to comment transaction
  bi->setComment("Keepalive check for " + _pair.toStdString());
  
  // Set destination
  bi->setDestination(_pair.second().mExternalAddr); 
  
  // Set component port
  bi->setComponent(_pair.first().mComponentId);

  // Put transaction to container
  mActiveChecks.addRegular(bi);
#endif
}

bool Stream::processSdpOffer( std::vector<std::string>& candidateList, std::string defaultIP, unsigned short defaultPort, bool deleteRelayed)
{
  if (candidateList.empty())
  {
    // Anyway we may need TURN permissions installed
    if (mState >= CreatingSDP && mConfig.mUseTURN)
    {
      ICELogDebug(<< "Installing permissions for default address " << defaultIP << ":" << defaultPort);
      installPermissions(-1, NetworkAddress(defaultIP, defaultPort));
    }
    return false;
  }

  // Save remote candidate list
  std::set<int> availComponents;

  for (size_t i=0; i<candidateList.size(); i++)
  {
    Candidate cand = Candidate::parseSdp(candidateList[i].c_str());
    
    if (availComponents.find(cand.mComponentId) == availComponents.end())
      availComponents.insert(cand.mComponentId);

    unsigned existingIndex = findRemoteCandidate(cand.mExternalAddr, cand.mComponentId);
    if (existingIndex != NoIndex)
    {
      ICELogInfo(<< "Stack ID " << mStackId << ". Remote candidate " << cand.mExternalAddr.toStdString() << " is found already, so just update priority value.");
      mRemoteCandidate[existingIndex].mPriority = cand.mPriority;
    }
    else
    {
      ICELogInfo(<< "Stack ID " << mStackId << ". Remote candidate " << cand.mExternalAddr.toStdString() << " is add.");
      mRemoteCandidate.push_back(cand);
    }
  }
  
  // Update number of components according to sdp
  ComponentMap validComponentMap;
  
  ComponentMap::iterator componentIter = mComponentMap.begin();
  for (; componentIter != mComponentMap.end(); componentIter++)
  {
    if (availComponents.find(componentIter->first) != availComponents.end())
      validComponentMap[componentIter->first] = componentIter->second;
    else
    {
      // Do not deallocate turn candidate here - side effect can be bad
    }
  }
  mComponentMap = validComponentMap;

  // Ensure there is at least one component
  if (mComponentMap.empty())
  {
    ICELogCritical(<< "SDP offer is not valid; it has no components");
    return false;
  }
  
  // Check if candidate list includes default IP:pair
  bool result = candidateListContains(defaultIP, defaultPort);

  // Delete relayed candidates if needed
  if (deleteRelayed)
  {
    mRemoteRelayedCandidate.clear();
    CandidateVector::iterator candIter = mRemoteCandidate.begin();
    while (candIter != mRemoteCandidate.end())
    {
      if (candIter->mType == Candidate::ServerRelayed)
      {
        mRemoteRelayedCandidate.push_back(*candIter);
        candIter = mRemoteCandidate.erase(candIter);
      }
      else
        candIter++;
    }
  }

  return result;
}

bool Stream::candidateListContains(const std::string& remoteIP, unsigned short remotePort)
{
  NetworkAddress remote;
  remote.setIp(remoteIP.c_str());
  remote.setPort(remotePort);

  for (size_t i=0; i<mRemoteCandidate.size(); i++)
    if (mRemoteCandidate[i].mExternalAddr == remote)
      return true;

  return false;
}

void Stream::createCheckList()
{
  // Iterate all components
  ComponentMap::iterator componentIter;
  for (componentIter = mComponentMap.begin(); componentIter != mComponentMap.end(); componentIter++)
  {
    // Pair local candidates with remote candidates
    for (unsigned localIndex = 0; localIndex < mLocalCandidate.size() && localIndex < ICE_CANDIDATE_LIMIT; localIndex++)
    {
      Candidate& local = mLocalCandidate[localIndex];
      if (local.mComponentId != (int)componentIter->first || !local.mReady )
        continue;

      for (unsigned remoteIndex=0; remoteIndex<mRemoteCandidate.size() && remoteIndex < ICE_CANDIDATE_LIMIT; remoteIndex++)
      {
        Candidate& remote = mRemoteCandidate[remoteIndex];
          
        // Ensure both candidates use the same transport and belongs to the same component ID
        if (local.mComponentId != remote.mComponentId || local.mExternalAddr.family() != remote.mExternalAddr.family())
          continue;

        if (local.mType == Candidate::ServerRelayed && !remote.mExternalAddr.isPublic())
          continue;
        
#ifdef ICE_SKIP_RELAYED_CHECKS
        if (local.mType == Candidate::ServerRelayed)
          continue;
#endif

        CandidatePair pair;

        pair.first() = local;
        pair.second() = remote;
        if (mAgentRole == RoleControlled)
        {
          pair.setControlledIndex(0);
          pair.setControllingIndex(1);
        }
        else
        if (mAgentRole == RoleControlling)
        {
          pair.setControlledIndex(1);
          pair.setControllingIndex(0);
        }
        else
          assert(0);
        
        pair.updatePriority();
        pair.updateFoundation();
        mCheckList.add(pair);
      }
    }
  }
  
  // Sort

  // Replace server reflexive candidates with their bases + prune list
  mCheckList.prune(ICE_CONNCHECK_LIMIT);


  ICELogInfo(<< "Stack ID: " << mStackId << ". Created check list: \n" << mCheckList.toStdString());
}

void 
Stream::startChecks()
{
  // Set state to connectivity checks
  mState = ConnCheck;
  mErrorCode = 0;
  
  // Checklist should not be cleared here - at this moment peer reflexive candidates can exists
  // List of TURN bound channels should not be cleared here too - this binding can still exists on relay
  //mCheckList.clear();

  // Create TURN permissions
  if (mConfig.mUseTURN)
    installPermissions();

  // Create check list
  createCheckList();

  // Create transactions for checklist
  runCheckList(CandidatePair::Regular, CandidatePair::Waiting);

  // Start send timer
  mScheduleTimer.start(ICE_SCHEDULE_TIMER_INTERVAL);
}

class DeleteChecksAndGathers: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    ClientRefresh* c = dynamic_cast<ClientRefresh*>(t);
    if (c)
      return false;
    ClientChannelBind* ccb = dynamic_cast<ClientChannelBind*>(t);
    if (ccb)
      return false;
    ClientCreatePermission* ccp = dynamic_cast<ClientCreatePermission*>(t);
    if (ccp)
      return false;
    
    // Send indication is not listed here; it is sent directly without transaction list proxy.
    return true;
  }
};

void Stream::stopChecks()
{
  // Clear generated check list
  mCheckList.clear();
  
  // Delete corresponding transactions
  mActiveChecks.erase(DeleteChecksAndGathers());
  
  // Avoid responding to remote peer's transactions
  if (mState < Failed)
    mState = Failed;
}



void Stream::clear()
{
  ICELogDebug(<< "Clear ICE stream");
  mState = None;
  
  // Clear connectivity checks list
  mCheckList.clear();
  
  // Delete all transactions except possible deallocation requests
  mActiveChecks.erase(KeepDeallocationRequest());
  
  // Remote candidates are not needed anymore
  mRemoteCandidate.clear();
  mRemoteRelayedCandidate.clear();
  mDefaultCandidate.clear();
  mComponentLimit.clear();
  mTurnPrefixMap.clear();
  mScheduleTimer.stop();
  mTurnAllocated = 0;
  mDefaultIPChanged = false;
  for (ComponentMap::iterator componentIter = mComponentMap.begin(); componentIter != mComponentMap.end(); componentIter++)
    componentIter->second.mNominationWaitIntervalStartTime = 0;
  mNominationWaitStartTime = 0; 
  mFailoverRequestsFinished = 0;
}

class DeleteConnChecks: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return (dynamic_cast<ConnectivityCheck*>(t) != NULL);
  }
};

class DeleteChannelBindingRequests: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return (dynamic_cast<ClientChannelBind*>(t) != NULL);
  }
};

class DeleteClientRefreshes: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return (dynamic_cast<ClientRefresh*>(t) != NULL);
  }
};

class DeleteCreatePermissions: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return (dynamic_cast<ClientCreatePermission*>(t) != NULL);
  }
};

class DeleteClientBindings: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return dynamic_cast<ClientBinding*>(t) != NULL;
  }
};

class DeleteClientAllocations: public TransactionCondition
{
public:
  bool check(Transaction* t) const
  {
    return dynamic_cast<ClientAllocate*>(t) != NULL;
  }
};

void Stream::clearForRestart(bool localNetworkChanged)
{
  ICELogDebug(<< "Clear ice stream before restart");
  mState = None;
  mCheckList.clear();
  mRemoteCandidate.clear();
  mRemoteRelayedCandidate.clear();
  mTurnAllocated = 0;
  
  // Here deleting transactions common for remote or local peer changing networks
  
  // Existing connectivity checks must be removed - there is no sense in result from them
  mActiveChecks.erase(DeleteConnChecks());

  // Existing channel binding requests are useless now - new ones should be sent - with new IP:port (when remote peer changes) or after creation new socket(s) (when local peer changes)
  mActiveChecks.erase(DeleteChannelBindingRequests());
  
  // And there is no sense in old TURN permissions
  mActiveChecks.erase(DeleteCreatePermissions());
  
  if (localNetworkChanged)
  {
    // No sense to refresh current TURN allocation - old socket is lost, there is no way to send from the same ip:port pair exactly
    mActiveChecks.erase(DeleteClientRefreshes());
    
    // Maybe new candidates were gathering when network changed - so remove old ClientBinding requests
    mActiveChecks.erase(DeleteClientBindings());
    
    // The same about TURN allocations - delete allocation requests.
    mActiveChecks.erase(DeleteClientAllocations());
  }
}

void Stream::restart()
{
  mErrorCode = 0;
  if (mState > CreatingSDP)
    mState = ConnCheck;
  else
    mState = None;
	
  mCheckList.clear();
}

NetworkAddress Stream::remoteAddress(int componentID)
{
  NetworkAddress result;
  PCandidatePair highestPair = mCheckList.findHighestNominatedPair(componentID);
  if (highestPair)
  {
    result = highestPair->second().mExternalAddr;
    result.setRelayed(highestPair->first().mType == Candidate::ServerRelayed);
  }

  return result;
}

NetworkAddress Stream::localAddress(int component)
{
  PCandidatePair highestPair = mCheckList.findHighestNominatedPair(component);
  if (highestPair)
    return highestPair->first().mExternalAddr;
  else
    return NetworkAddress();
}

class FreeAllocationAction: public Action
{
protected:
  DeleteAllocationCallback* mCallback;
public:
  FreeAllocationAction(Stream& stream, StackConfig& config, DeleteAllocationCallback* cb)
    :Action(stream, config), mCallback(cb)
  {

  }
  ~FreeAllocationAction()
  {
    delete mCallback;
  }

  void finished(Transaction& transaction)
  {
    AuthTransaction& auth = dynamic_cast<AuthTransaction&>(transaction);
    int code = auth.state() == Transaction::Failed ? (auth.errorCode() ? auth.errorCode() : -1) : 0;
    
    if (mCallback)
      mCallback->onAllocationDeleted(mStream.mId, transaction.component(), code);
  }
};

void Stream::freeAllocation(int component, DeleteAllocationCallback* cb)
{
  // Clear bindings
  removeBindingResult(component);

  if (component == -1)
  {
    ComponentMap::iterator compIter;
    for (compIter = mComponentMap.begin(); compIter != mComponentMap.end(); ++compIter)
      freeAllocation(compIter->first, cb);
  }
  else
  {
    ClientRefresh* allocation = new ClientRefresh(0, this);
    allocation->setStackId( mStackId );
    allocation->setDestination( (mConfig.mServerAddr4.isZero() || mConfig.mServerAddr4.isEmpty()) ? mConfig.mServerAddr6 : mConfig.mServerAddr4);
    allocation->setPassword( mConfig.mTurnPassword );
    allocation->setUsername( mConfig.mTurnUsername );
    allocation->setComponent( component );
    allocation->setKeepalive( false );
#ifdef ICE_CACHE_REALM_NONCE
    allocation->setRealm( mCachedRealm );
    allocation->setNonce( mCachedNonce );
#endif
    // Associate it with action
    allocation->setAction(PAction(new FreeAllocationAction(*this, mConfig, cb)));

    mActiveChecks.addRegular(allocation);
  }
}

class ChannelBindResultAction: public Action
{
protected:
  ChannelBoundCallback* mCallback;

public:
  ChannelBindResultAction(Stream& stream, StackConfig& config, ChannelBoundCallback* cb)
  :Action(stream, config), mCallback(cb)
  {}

  ~ChannelBindResultAction()
  {
    delete mCallback;
  }

  void finished(Transaction& transaction)
  {
    ClientChannelBind& cb = dynamic_cast<ClientChannelBind&>(transaction);
    Stream::BoundChannel channel;
    channel.mComponentId = cb.component();
    channel.mPrefix = cb.channelPrefix();
    channel.mPeerAddress = cb.peerAddress();
    channel.mResultCode = 0;
    
    if (cb.state() == Transaction::Failed)
      channel.mResultCode = cb.errorCode() ? cb.errorCode() : -1;
    
    ICELogDebug(<< "Saving TURN channel binding result " << channel.mPrefix << " to " << channel.mPeerAddress.toStdString());
    mStream.mBoundChannelList.push_back(channel);
    if (mCallback)
      mCallback->onChannelBound(mStream.mId, cb.component(), channel.mResultCode);
  }
};

TurnPrefix Stream::bindChannel(const NetworkAddress& peerAddress, int component, ChannelBoundCallback* cb)
{
  ICELogInfo(<< "Bind TURN channel for " << peerAddress.toStdString() << " for component " << component);
  
  // Check if the request binding is done already
  BoundChannelList::iterator channelIter = mBoundChannelList.begin();
  for (;channelIter != mBoundChannelList.end(); ++channelIter)
  {
    if (channelIter->mComponentId == component && channelIter->mPeerAddress == peerAddress)
    {
      if (!channelIter->mResultCode)
      {
        ICELogInfo(<< "Already bound to prefix " << channelIter->mPrefix);
        return channelIter->mPrefix;
      }
    }
  }
  
  ChannelBindResultAction* action = new ChannelBindResultAction(*this, mConfig, cb);

  // Create transaction
  ClientChannelBind* ccb = new ClientChannelBind( peerAddress );
  ccb->setStackId( mStackId );

  ccb->setUsername( mConfig.mTurnUsername );
  ccb->setPassword( mConfig.mTurnPassword );
  ccb->setDestination( mConfig.mUseIPv6 && !mConfig.mUseIPv4 ? mConfig.mServerAddr6 : mConfig.mServerAddr4 );
  ccb->setComponent( component );
#ifdef ICE_CACHE_REALM_NONCE
  ccb->setRealm( mCachedRealm );
  ccb->setNonce( mCachedNonce );
#endif
  ccb->setAction(PAction(action));
  mActiveChecks.addRegular(ccb);

  return ccb->channelPrefix();
}

bool Stream::isChannelBindingFailed(int component, TurnPrefix prefix)
{
  BoundChannelList::iterator channelIter = mBoundChannelList.begin();
  for (; channelIter != mBoundChannelList.end(); ++channelIter)
      if (channelIter->mComponentId == component && channelIter->mPrefix == prefix)
        return channelIter->mResultCode != 0;
  
  return false;
}

void Stream::removeBindingResult(int component)
{
  // Clear related channel bindings
  Stream::BoundChannelList::iterator channelIter = mBoundChannelList.begin();
  while (channelIter != mBoundChannelList.end())
  {
    if (channelIter->mComponentId == component)
    {
      ICELogDebug(<< "Clearing TURN channel binding " << channelIter->mPrefix << " for " << channelIter->mPeerAddress.toStdString());
      channelIter = mBoundChannelList.erase(channelIter);
    }
    else
      channelIter++;
  }
}


NetworkAddress Stream::reflexiveAddress(int componentID)
{
  for (size_t i=0; i<mLocalCandidate.size(); i++)
  {
    if (mLocalCandidate[i].mType == Candidate::ServerReflexive && mLocalCandidate[i].mComponentId == componentID)
      return mLocalCandidate[i].mExternalAddr;
  }

  return NetworkAddress();
}

NetworkAddress Stream::relayedAddress(int componentID)
{
  for (size_t i=0; i<mLocalCandidate.size(); i++)
  {
    if (mLocalCandidate[i].mType == Candidate::ServerRelayed && mLocalCandidate[i].mComponentId == componentID)
      return mLocalCandidate[i].mExternalAddr;
  }
  
  return NetworkAddress();
}

NetworkAddress Stream::remoteRelayedAddress(int component)
{
  for (size_t i=0; i<mRemoteCandidate.size(); i++)
  {
    if (mRemoteCandidate[i].mType == Candidate::ServerRelayed && mRemoteCandidate[i].mComponentId == component)
      return mRemoteCandidate[i].mExternalAddr;
  }
  
  for (size_t i=0; i<mRemoteRelayedCandidate.size(); i++)
  {
    if (mRemoteRelayedCandidate[i].mType == Candidate::ServerRelayed && mRemoteRelayedCandidate[i].mComponentId == component)
      return mRemoteRelayedCandidate[i].mExternalAddr;
  }
  
  return NetworkAddress();
}

NetworkAddress Stream::remoteReflexiveAddress(int component)
{
  for (size_t i=0; i<mRemoteCandidate.size(); i++)
  {
    if (mRemoteCandidate[i].mType == Candidate::ServerReflexive && mRemoteCandidate[i].mComponentId == component)
      return mRemoteCandidate[i].mExternalAddr;
  }
  
  return NetworkAddress();
}

class ICEBindAction: public Action
{
public:
  virtual void finished(Transaction& st)
  {
    ICELogInfo(<< "Stack ID " << st.stackId() << ". ChannelBind transaction is finished.");
  }
};


void Stream::installPermissions(int component, const NetworkAddress& addr, InstallPermissionsCallback* cb)
{
  // Here should be check if there are all components allocated and if remote candidates are known.
  // It is useless for now - installPermissions() is called only when starting connectivity checks - components & remote candidates are here already.
  ICELogInfo( << "Stack ID " << mStackId << ". Attempt to install TURN client permissions.");

  // Iterate all components
  int requestCounter = 0;
  for (ComponentMap::iterator cmip = mComponentMap.begin(); cmip != mComponentMap.end(); ++cmip)
  {
    // Check if component is found
    if (component != cmip->first && component != -1)
      continue;

    ClientCreatePermission* ccp = new ClientCreatePermission();

    ccp->setComment( "CreatePermissions request" );
    ccp->setStackId( mStackId );
    ccp->setDestination( (mConfig.mServerAddr4.isEmpty() || mConfig.mServerAddr4.isZero()) ? mConfig.mServerAddr6 : mConfig.mServerAddr4 );
    ccp->setUsername( mConfig.mTurnUsername );
    ccp->setPassword( mConfig.mTurnPassword );
    
#ifdef ICE_CACHE_REALM_NONCE
    ccp->setRealm( mCachedRealm );
    ccp->setNonce( mCachedNonce );
#endif
    ccp->setAction( PAction(new ICEInstallPermissionsAction(*this, mConfig, cb)) );

    // Get remote candidates for given component
    if (addr.isEmpty())
    {
      for (auto candIter = mRemoteCandidate.begin(); candIter != mRemoteCandidate.end(); candIter++)
        if (candIter->mComponentId == cmip->first && candIter->mExternalAddr.isPublic())
          ccp->addIpAddress(candIter->mExternalAddr);
      for (auto candIter = mRemoteRelayedCandidate.begin(); candIter != mRemoteRelayedCandidate.end(); candIter++)
        if (candIter->mComponentId == cmip->first)
          ccp->addIpAddress(candIter->mExternalAddr);
    }
    else
      ccp->addIpAddress(addr);
    ccp->setComponent(cmip->first);
    
    // Enqueue transaction
    mActiveChecks.addPrioritized(ccp);
    requestCounter++;
  }
  ICELogDebug(<< "Created " << requestCounter << " CreatePermission requests");
}

void Stream::dump(std::ostream& output)
{
  output << Logger::TabPrefix << "State: " << RunningStateToString(mState) << std::endl;
  output << Logger::TabPrefix << "Local candidates: " << std::endl;
  for (unsigned i=0; i<mLocalCandidate.size(); i++)
    mLocalCandidate[i].dump(output);
  output << Logger::TabPrefix << "Remote candidates: " << std::endl;
  for (unsigned i=0; i<mRemoteCandidate.size(); i++)
    mRemoteCandidate[i].dump(output);
  for (unsigned i=0; i<mRemoteRelayedCandidate.size(); i++)
    mRemoteRelayedCandidate[i].dump(output);
  output << Logger::TabPrefix << "Check list:" << std::endl;
  mCheckList.dump(output);
}

bool Stream::findConcludePair(Candidate& local, Candidate& remote)
{
  PCandidatePair validPair = mCheckList.findBestValidPair(local.mComponentId);
  if (!validPair)
    return false;
  local = validPair->first();
  remote = validPair->second();
  return true;
}

int Stream::findComponentIdByPort(unsigned short port, int family)
{
  ComponentMap::iterator it = mComponentMap.begin();
  for (;it != mComponentMap.end();it++)
    if (family == AF_INET)
    {
      if (it->second.mPort4 == port)
        return it->first;
    }
    else
    {
      if (it->second.mPort6 == port)
        return it->first;
    }

  return -1;
}

void Stream::clearChecks()
{
  switch (mState)
  {
  case Success:
    mCheckList.clear();
    mActiveChecks.erase(EraseBindingAndRelaying());
    break;

  case Failed:
    mCheckList.clear();
    mActiveChecks.erase(KeepDeallocationRequest());
    break;
      
  default:
    break;
  }
}

void Stream::dumpLocalCandidateList()
{
  std::ostringstream oss;
  oss << "Local candidate list:" << std::endl;
  for (unsigned i=0; i<mLocalCandidate.size(); i++)
  {
    Candidate& c = mLocalCandidate[i];
    oss << " ";c.dump(oss);
  }
  ICELogDebug( << oss.str() );
}

void Stream::processStateChain()
{
  if (mState == EliminateRedudand)
    Handle_ER();
  if (mState == ComputingFoundations)
    Handle_CF();
  if (mState == StartingKeepAlives)
    Handle_SKA();
  if (mState == PrioritizingCandidates)
    Handle_PC();
  if (mState == ChoosingDefault)
    Handle_CD();
}

void Stream::unfreeze(const char* foundation)
{
  for (unsigned i=0; i<mCheckList.count(); i++)
  {
    CandidatePair& c = *mCheckList[i];
    if (c.foundation() == foundation && c.state() == CandidatePair::Frozen /*&& c.role() == r*/)
      c.setState(CandidatePair::Waiting);
  }
}

class GatheringCondition: public TransactionCondition
{
protected:
  int mComponent;
  int mFailoverId;
  Transaction* mValidTransaction;
public:
  GatheringCondition(int component, int failoverId, Transaction* valid)
  :mComponent(component), mFailoverId(failoverId), mValidTransaction(valid)
  {}
  
  bool check(Transaction* t) const
  {
    if (!t)
      return false;
    if (t->removed())
      return false;
    if (!t->action())
      return false;
    if (t == mValidTransaction)
      return false;
    if (t->component() != mComponent)
      return false;
    if (t->failoverId() != mFailoverId)
      return false;
    
    // Special case for Allocate
    RelayingAction* relayingAction = dynamic_cast<RelayingAction*>(t->action().get());
    bool binding = dynamic_cast<BindingAction*>(t->action().get()) != NULL;
    
    if (relayingAction)
      relayingAction->autoreleaseAllocation();
    
    return binding;
  }
};

void Stream::removeGatherRequests(int component, int failoverId, Transaction* validOne)
{
  mActiveChecks.erase(GatheringCondition(component, failoverId, validOne));
}

void Stream::nominatePair(PCandidatePair &p)
{
  // Check if pair is already nominated
  if (p->nomination() != p->Nomination_None)
    return;
  
  if (!p->transaction())
  {
    // It can be newly created candidate pair
    PCandidatePair equalPair = mCheckList.findEqualPair(*p, CheckList::CT_Strict);
    if (equalPair)
      createCheckRequest(equalPair);
  }
  
  Transaction* t = reinterpret_cast<Transaction*>(p->transaction());
  if (!t)
  {
    ICELogCritical(<< "Logical error in nomination procedure.");
    return;
  }
  
  CheckResult* cr = dynamic_cast<CheckResult*>(t);

  p->setNomination(CandidatePair::Nomination_Started);
  
  // Restart binding transaction
  t->restart();
  
  
  // "upgrade" binding transaction to include USE-CANDIDATE attribute
  cr->useCandidate();
  
  // Push log records
  t->setComment("Nominated request for " + p->toStdString());
  ICELogInfo (<< "Stack ID " << t->stackId() << ". Generated nominated candidate request for pair " << p->toStdString());
  
  // Prioritize restarted transaction
  mActiveChecks.prioritize(t);
  
  // Mark transaction's action as nomination
  CheckPairAction* cp = dynamic_cast<CheckPairAction*>(t->action().get());
  if (cp)
    cp->setNomination(true);
}

class OldAllocationCondition: public TransactionCondition
{
protected:
  int mComponent;
public:
  OldAllocationCondition(int component)
  :mComponent(component)
  {}
  
  bool check(Transaction* t) const
  {
    if (!t->removed())
      return false;
    ClientRefresh* cf = dynamic_cast<ClientRefresh*>(t);
    if (!cf)
      return false;
    return true;
  }
};
bool Stream::ressurectAllocation(int component)
{
  std::vector<Transaction*> allocations;
  mActiveChecks.copyMatching(OldAllocationCondition(component), allocations);
  if (allocations.empty())
    return false;
  // Find most recent allocation
  unsigned current = ICETimeHelper::timestamp();
  unsigned delta = 0xFFFFFFFF;
  Transaction* chosen = NULL;
  for (size_t index=0; index<allocations.size(); index++)
  {
    Transaction* t = allocations[index];
    if (current - t->timestamp() < delta)
    {
      chosen = t;
      delta = current - t->timestamp();
    }
  }
  
  if (!chosen)
    return false;
  
  ClientRefresh* cf = dynamic_cast<ClientRefresh*>(chosen);
  if (!cf)
    return false; // Just extra check
  cf->setRemoved(false);
  
  // Update local candidates
  for (unsigned i=0; i<mLocalCandidate.size(); i++)
  {
    Candidate& c = mLocalCandidate[i];
    
    // Avoid updating of successful candidates to failed state
    if (c.mComponentId == component)
    {
      c.mReady = true;
      c.mFailed = false;
      if (c.mType == Candidate::ServerReflexive)
        c.mExternalAddr = cf->reflexiveAddress();
      else
      if (c.mType == Candidate::ServerRelayed)
        c.mExternalAddr = cf->relayedAddress();
    }
  }
  return true;
}
