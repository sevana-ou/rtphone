/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_PROVIDER_H
#define __AUDIO_PROVIDER_H

#include "EP_DataProvider.h"
#include "../helper/HL_InternetAddress.h"
#include "../helper/HL_Rtp.h"
#include "../media/MT_Box.h"
#include "../media/MT_Stream.h"
#include "../media/MT_Codec.h"
#include "../audio/Audio_Interface.h"

#include "rutil/ThreadIf.hxx"
#include <vector>
#include <string>

class UserAgent;

class AudioProvider: public DataProvider
{
public:

  AudioProvider(UserAgent& agent, MT::Terminal& terminal);
  virtual ~AudioProvider();
  
  // Returns provider RTP name
  std::string   streamName();
  
  // Returns provider RTP profile name
  std::string   streamProfile();
  
  // Sets destination IP address
  void          setDestinationAddress(const RtpPair<InternetAddress>& addr);

  // Processes incoming data
  void          processData(PDatagramSocket s, const void* dataBuffer, int dataSize, InternetAddress& source);

  // This method is called by user agent to send ICE packet from mediasocket
  void          sendData(PDatagramSocket s, InternetAddress& destination, const void* dataBuffer, unsigned int datasize);

  // Updates SDP offer
  void          updateSdpOffer(resip::SdpContents::Session::Medium& sdp, SdpDirection direction);

  // Called by user agent when session is deleted.
  void          sessionDeleted();

  // Called by user agent when session is terminated.
  void          sessionTerminated();

  // Called by user agent when session is started.
  void          sessionEstablished(int conntype);

  // Called by user agent to save media socket for this provider
  void          setSocket(const RtpPair<PDatagramSocket>& p4, const RtpPair<PDatagramSocket>& p6);
  
  // Called by user agent to get media socket for this provider
  RtpPair<PDatagramSocket>& socket(int family);

  // Called by user agent to process media stream description from remote peer.
  // Returns true if description is processed succesfully. Otherwise method returns false.
  // myAnswer sets if the answer will be sent after.
  bool          processSdpOffer(const resip::SdpContents::Session::Medium& media, SdpDirection sdpDirection);


  void          setState(unsigned state) override;
  unsigned      state();
  MT::Statistics  getStatistics();
  MT::PStream   activeStream();

  void readFile(const Audio::PWavFileReader& stream, MT::Stream::MediaDirection direction);
  void writeFile(const Audio::PWavFileWriter& stream, MT::Stream::MediaDirection direction);
  void setupMirror(bool enable);

  void configureMediaObserver(MT::Stream::MediaObserver* observer, void* userTag);

protected:
  // SDP's stream name
  std::string             mStreamName;
    
  // Socket handles to operate
  RtpPair<PDatagramSocket>   mSocket4, mSocket6;

  // Destination IP4/6 address
  RtpPair<InternetAddress>         mDestination;
  
  MT::PStream mActiveStream;
  UserAgent& mUserAgent;
  MT::Terminal& mTerminal;
  MT::Statistics mBackupStats;

  unsigned mState;
  SrtpSuite mSrtpSuite;
  struct RemoteCodec
  {
    RemoteCodec(MT::Codec::Factory* factory, int payloadType)
      :mFactory(factory), mRemotePayloadType(payloadType)
    { }

    MT::Codec::Factory* mFactory;
    int mRemotePayloadType;
  };
  std::vector<RemoteCodec> mAvailableCodecs;
  int mRemoteTelephoneCodec;              // Payload type of remote rfc2833 codec
  bool mRemoteNoSdp;                      // Marks if we got no-sdp offer
  MT::CodecListPriority mCodecPriority;
  MT::Stream::MediaObserver* mMediaObserver = nullptr;
  void* mMediaObserverTag = nullptr;

  std::string createCryptoAttribute(SrtpSuite suite);
  SrtpSuite processCryptoAttribute(const resip::Data& value, ByteBuffer& key);
  void findRfc2833(const resip::SdpContents::Session::Medium::CodecContainer& codecs);

  // Implements setState() logic. This allows to be called from constructor (it is not virtual function)
  void setStateImpl(unsigned state);

};

#endif
