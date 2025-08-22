/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_NATIVE_RTP_SENDER_H
#define __MT_NATIVE_RTP_SENDER_H

#include "../engine_config.h"
#include "jrtplib/src/rtpexternaltransmitter.h"
#include "libsrtp/include/srtp.h"
#include "../helper/HL_NetworkSocket.h"
#include "../helper/HL_InternetAddress.h"
#include "../helper/HL_Rtp.h"
#include "../helper/HL_SocketHeap.h"
#include "MT_Stream.h"
#include "MT_SrtpHelper.h"

namespace MT
{
  class NativeRtpSender: public jrtplib::RTPExternalSender
  {
  public:
    NativeRtpSender(Statistics& stat);
    ~NativeRtpSender();
    
	  /** This member function will be called when RTP data needs to be transmitted. */
	  bool SendRTP(const void *data, size_t len);

	  /** This member function will be called when an RTCP packet needs to be transmitted. */
	  bool SendRTCP(const void *data, size_t len);

	  /** Used to identify if an RTPAddress instance originated from this sender (to be able to detect own packets). */
    bool ComesFromThisSender(const jrtplib::RTPAddress *a);
    
    void setDestination(RtpPair<InternetAddress> destination);
    RtpPair<InternetAddress> destination();

    void setSocket(const RtpPair<PDatagramSocket>& socket);
    RtpPair<PDatagramSocket>& socket();
    
#if defined(USE_RTPDUMP)
    void setDumpWriter(RtpDump* dump);
    RtpDump* dumpWriter();
#endif
    void setSrtpSession(SrtpSession* srtp);
    SrtpSession* srtpSession();

  protected:
    RtpPair<PDatagramSocket> mSocket;
    RtpPair<InternetAddress> mTarget;
    Statistics& mStat;
#if defined(USE_RTPDUMP)
    RtpDump* mDumpWriter = nullptr;
#endif
    SrtpSession* mSrtpSession;
    char mSendBuffer[MAX_VALID_UDPPACKET_SIZE];
  };
}

#endif
