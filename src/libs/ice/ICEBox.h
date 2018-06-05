/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __NAT_ICE_H
#define __NAT_ICE_H

#include <string>
#include <vector>

#include "ICESync.h"
#include "ICEEvent.h"
#include "ICEAddress.h"
#include "ICEByteBuffer.h"
#include "ICECandidate.h"

#define ICE_TIMEOUT 8000
#define ICE_CHECK_INTERVAL 20

#define ICE_RTP_ID 1
#define ICE_RTCP_ID 2

namespace ice 
{

  // Structure to describe used STUN/TURN configuration
  struct ServerConfig
  {
    std::vector<NetworkAddress> mServerList4, mServerList6; // List of STUN/TURN servers for IPv4 and IPv6 protocols
    bool              mUseIPv4;         // Marks if IPv4 should be used when gathering candidates
    bool              mUseIPv6;         // Marks if IPv6 should be used when gathering candidates
    std::string       mHost;            // Target host to get default IP interface; usually it is public IP address
    bool              mRelay;			      // Marks if TURN is to be used instead STUN
    unsigned int      mTimeout;			    // Timeout value in milliseconds
    unsigned int      mPeriod;			    // Transmission interval
	  std::string			  mUsername;				// User name for TURN server [optional]
	  std::string       mPassword;				// Password for TURN server [optional]
        
    ServerConfig()
      :mUseIPv4(true), mUseIPv6(true), mRelay(false), mTimeout(ICE_TIMEOUT), mPeriod(ICE_CHECK_INTERVAL)
    {
	  }

    ~ServerConfig()
    {}
  };
  
  enum IceState
  {
    IceNone         = 0,      // Stack non active now
    IceGathering    = 1,      // Stack gathering candidates now
    IceGathered     = 2,      // Stack gathered candidates
    IceChecking     = 3,      // Stack runs connectivity checks now
    IceCheckSuccess = 4,      // Connectivity checks were successful
    IceFailed       = 5,      // Connectivity checks or gathering failed
    IceTimeout      = 6       // Timeout
  };
  
  
  enum AgentRole
  {
    RoleControlled = 1,
    RoleControlling = 2
  };

  // ICE stack
  class Stack
  {
  public:
    static void    initialize();
    static void    finalize();

    static Stack* makeICEBox(const ServerConfig& config);
    
    // Service methods to check incoming packets
    static bool isDataIndication(ByteBuffer& source, ByteBuffer* plain);
    static bool isStun(ByteBuffer& source);
    static bool isRtp(ByteBuffer& data);
    static bool isChannelData(ByteBuffer& data, TurnPrefix prefix);

    static ByteBuffer makeChannelData(TurnPrefix prefix, const void* data, unsigned datasize);

    /*! Sets ICE event handler pointer in stack.
     *  @param handler Pointer to ICE event handler instance.
     *  @param tag Custom user tag. */
    virtual void   setEventHandler(StageHandler* handler, void* tag) = 0;
    
    /*! Adds new stream to ICE stack object.
     *  @return ID of created stream.
     */
    virtual int    addStream() = 0;

    /*! Adds new component (socket) to media stream. 
     *  @param portNumber specifies used local port number for the socket. 
     *  @returns component ID. This ID is unique only for specified stream. Two components in different streams can have the same */
    virtual int  addComponent(int streamID, void* tag, unsigned short port4, unsigned short port6) = 0;
    
    /*! Removes media stream from ICE stack object. */
    virtual void removeStream(int streamID) = 0;
    
	  virtual bool hasStream(int streamId) = 0;
    virtual bool hasComponent(int streamId, int componentId) = 0;
	  virtual void setComponentPort(int streamId, int componentId, unsigned short port4, unsigned short port6) = 0;

    virtual void setRole(AgentRole role) = 0;
    virtual AgentRole role() = 0;
		
    /*! Processes incoming data.
     *  @param streamID ICE stream ID
     *  @param componentID ICE component ID
     *  @param sourceIP IP of remote peer
     *  @param port number of remote peer
     *  @param sourceBuffer pointer to incoming data buffer
     *  @param sourceSize size of incoming data (in bytes)
     */
    virtual bool          processIncomingData(int stream, int component, ByteBuffer& incomingData) = 0;
 
    /*! Generates outgoing data for sending. 
     *  @param response marks if the returned packet is response packet to remote peer's request
     *  @param streamID ICE stream ID
     *  @param componentID ICE component ID
     *  @param destIP Character buffer to write destination IP address in textual form
     *  @param destPort Destination port number.
     *  @param dataBuffer Pointer to output buffer. It must be big enough to include at least 1500 bytes - it is biggest UDP datagram in this library. 
     */
    virtual PByteBuffer generateOutgoingData(bool& response, int& stream, int& component, void*& tag) = 0;
		
    /*! Searches stream&component IDs by used local port and socket family. */
    virtual bool          findStreamAndComponent(int family, unsigned short port, int* stream, int* component) = 0;

    /*! Starts to gather local candidates. */
    virtual void          gatherCandidates() = 0;
    
    /*! Starts ICE connectivity checks. */ 
    virtual void          checkConnectivity() = 0;

    /*! Checks if gathering is finished. */
    virtual IceState      state() = 0;
    
    /*! Creates common part of SDP. It includes ice-full attribute and ufrag/pwd pair. 
     *  @param common Common part of SDP. */
    virtual void createSdp(std::vector<std::string>& common) = 0;
    
    /*! Returns default address for specified stream/component ID.
     *  @param ip Default IP address.
     *  @param port Default port number. */
    virtual NetworkAddress defaultAddress(int streamID, int componentID) = 0;
    
    /*! Returns candidate list for specified stream/component ID.
     *  @param streamID Stream ID.
     *  @param componentID Component ID.
     *  @param candidateList Output vector of local candidates. */
    virtual void fillCandidateList(int streamID, int componentID, std::vector<std::string>& candidateList) = 0;

    /*! Process ICE offer text for specified stream.
     *  @param streamIndex ICE stream index.
     *  @param candidateList Input vector of strings - it holds candidate list.
     *  @param defaultIP Default IP for component ID 0.
     *  @param defaultPort Default port number for component ID 0. 
     *  @return Returns true if processing(parsing) was ok, otherwise method returns false. */
    virtual bool processSdpOffer(int streamIndex, std::vector<std::string>& candidateList,
                                 const std::string& defaultIP, unsigned short defaultPort, bool deleteRelayed) = 0;
    
    virtual NetworkAddress getRemoteRelayedCandidate(int stream, int component) = 0;
    virtual NetworkAddress getRemoteReflexiveCandidate(int stream, int component) = 0;
    
    /*! Notifies stack about ICE password of remote peer.
     *  @param pwd ICE password. */
    virtual void setRemotePassword(const std::string& pwd, int streamId = -1) = 0;
    virtual std::string remotePassword(int streamId = -1) const = 0;

    /*! Notifies stack about ICE ufrag of remote peer.
     *  @param ufrag ICE ufrag credential. */
    virtual void setRemoteUfrag(const std::string& ufrag, int streamId = -1) = 0;
    virtual std::string remoteUfrag(int streamId = -1) const = 0;

    /*! Returns local ICE password.
     *  @return ICE password. */
    virtual std::string localPassword() const = 0;
    
    /*! Returns local ICE ufrag.
     *  @return ICE ufrag credential. */
    virtual std::string localUfrag() const = 0;

		/*! Checks if the specified value is ICE session's TURN prefix value. 
		 *  @return Returns true if parameter is TURN prefix value, false otherwise. */ 
		virtual bool hasTurnPrefix(unsigned short prefix) = 0;
		
		/*! Gets the discovered during connectivity checks remote party's address.
		 *	@return Remote party's address */
		virtual NetworkAddress remoteAddress(int stream, int component) = 0; 
    virtual NetworkAddress localAddress(int stream, int component) = 0;
		
    /*! Seeks for conclude pair. 
		 */
		virtual bool findConcludePair(int stream, Candidate& local, Candidate& remote) = 0;

		/*! Checks if remote candidate list contains specified address. 
		 *  @return True if address is found in remote candidate list, false otherwise. */
    virtual bool candidateListContains(int stream, const std::string& remoteIP, unsigned short remotePort) = 0;
    
    // Dumps current state of stack to output
    virtual void dump(std::ostream& output) = 0;

    // Returns if ICE session must be restarted after new offer.
    virtual bool mustRestart() = 0;
    
    // Clears all connectivity checks and reset state of session to None. It does not delete streams and components.
    virtual void clear() = 0;
    
    // Prepares stack to restart - clears remote candidate list, cancels existing connectivity checks, resets turn allocation counter
    virtual void clearForRestart(bool localNetworkChanged) = 0;
    
    // Stops all connectivity & gathering checks.
    virtual void stopChecks() = 0;
    
    // Refreshes local password and ufrag values. Useful when connectivity checks must be restarted.
    virtual void refreshPwdUfrag() = 0;
    
    // Binds channel and return channel prefix
    virtual TurnPrefix bindChannel(int stream, int component, const NetworkAddress& target, ChannelBoundCallback* cb) = 0;
    virtual bool isChannelBindingFailed(int stream, int component, TurnPrefix prefix) = 0;

    virtual void installPermissions(int stream, int component, const NetworkAddress& address, InstallPermissionsCallback* cb) = 0;

    // Starts freeing of TURN allocations
    virtual void freeAllocation(int stream, int component, DeleteAllocationCallback* cb) = 0;
    
    // Checks if there are active allocations on TURN server
    virtual bool hasAllocations() = 0;
    
    // Checks if any of stream has set error code and return it
    virtual int errorCode() = 0;
    
    // Returns the list of candidates from remote peer
    virtual std::vector<Candidate>* remoteCandidates(int stream) = 0;    
    
    // Returns chosen stun server address during last candidate gathering
    virtual NetworkAddress activeStunServer(int stream) const = 0;

    virtual void setup(const ServerConfig& config) = 0;
    virtual bool isRelayHost(const NetworkAddress& remote) = 0;
    virtual bool isRelayAddress(const NetworkAddress& remote) = 0;

    virtual ~Stack();
  };

} //end of namespace

#endif
