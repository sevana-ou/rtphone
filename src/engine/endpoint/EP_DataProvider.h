/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __DATA_PROVIDER_H
#define __DATA_PROVIDER_H

#include <string>
#include <vector>

#include "resip/stack/SdpContents.hxx"

#include "../helper/HL_InternetAddress.h"
#include "../helper/HL_NetworkSocket.h"
#include "../helper/HL_Pointer.h"
#include "../media/MT_Stream.h"

class DataProvider
{
public:
  enum MediaFlow
  {
    mfActive,
    mfPaused
  };
  
  enum MediaState
  {
    msSendRecv,
    msSendonly,
    msRecvonly,
    msInactive
  };

  static bool isSupported(const char* name);

  // Returns provider RTP name
  virtual std::string   streamName() = 0;
  
  // Returns provider RTP profile name
  virtual std::string   streamProfile() = 0;
  
  // Sets destination IP address
  virtual void          setDestinationAddress(const RtpPair<InternetAddress>& addr) = 0;

  // Processes incoming data
  virtual void          processData(const PDatagramSocket& s, const void* dataBuffer, int dataSize, InternetAddress& address) = 0;

  // This method is called by user agent to send ICE packet from mediasocket
  virtual void          sendData(const PDatagramSocket& s, InternetAddress& destination, const void* dataBuffer, unsigned int datasize) = 0;
  
  // Updates SDP offer
  virtual void          updateSdpOffer(resip::SdpContents::Session::Medium& sdp, SdpDirection direction) = 0;

  // Called by user agent when session is deleted. Comes after sessionTerminated().
  virtual void          sessionDeleted() = 0;

  // Called by user agent when session is terminated.
  virtual void          sessionTerminated() = 0;

  // Called by user agent when session is started.
  virtual void          sessionEstablished(int conntype) = 0;

  // Called by user agent to save media socket for this provider
  virtual void          setSocket(const RtpPair<PDatagramSocket>&  p4, const RtpPair<PDatagramSocket>& p6) = 0;
  
  // Called by user agent to get media socket for this provider
  virtual RtpPair<PDatagramSocket>& socket(int family) = 0;

  // Called by user agent to process media stream description from remote peer.
  // Returns true if description is processed succesfully. Otherwise method returns false.
  virtual bool          processSdpOffer(const resip::SdpContents::Session::Medium& media, SdpDirection sdpDirection) = 0;
  
  virtual unsigned      state() = 0;
  virtual void          setState(unsigned state) = 0;
  
  virtual void          pause();
  virtual void          resume();

  virtual MT::Statistics  getStatistics() = 0;

protected:
  MediaFlow  mActive;
  MediaState mRemoteState;
};

typedef std::shared_ptr<DataProvider>  PDataProvider;
typedef std::vector<PDataProvider>    DataProviderVector;

#endif
