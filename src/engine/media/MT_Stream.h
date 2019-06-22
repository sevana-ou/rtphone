/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_STREAM_H
#define __MT_STREAM_H

#include "ice/ICEAddress.h"
#include "MT_Codec.h"
#include "MT_SrtpHelper.h"
#include "MT_Statistics.h"
#include "../helper/HL_InternetAddress.h"
#include "../helper/HL_NetworkSocket.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_Rtp.h"
#include "../audio/Audio_WavFile.h"
#include "../audio/Audio_DataWindow.h"
#include <vector>
#include <map>
#include <chrono>
#include "../helper/HL_Optional.hpp"

using std::experimental::optional;

namespace MT
{
  class Stream
  {
  public:
    enum Type
    {
      Audio = 1,
      Video = 2
    };
    
    enum class MediaDirection
    {
      Incoming,
      Outgoing
    };

    class MediaObserver
    {
    public:
      virtual void onMedia(const void* buffer, int length, MT::Stream::MediaDirection direction,
                           void* context, void* userTag) = 0;
    };

    Stream();
    virtual ~Stream();

    virtual void setDestination(const RtpPair<InternetAddress>& dest);

    virtual void setTransmittingCodec(Codec::Factory& factory, int payloadType) = 0;    
    virtual void dataArrived(PDatagramSocket s, const void* buffer, int length, InternetAddress& source) = 0;


    virtual void readFile(const Audio::PWavFileReader& reader, MediaDirection direction) = 0;
    virtual void writeFile(const Audio::PWavFileWriter& writer, MediaDirection direction) = 0;
    virtual void setupMirror(bool enable) = 0;

    virtual void setState(unsigned state);
    virtual unsigned state();
    
    virtual void setSocket(const RtpPair<PDatagramSocket>& socket);
    virtual RtpPair<PDatagramSocket>& socket();

    Statistics& statistics();
    SrtpSession& srtp();
    void configureMediaObserver(MediaObserver* observer, void* userTag);

  protected:
    unsigned mState;
    RtpPair<InternetAddress> mDestination;
    RtpPair<PDatagramSocket> mSocket;
    Statistics mStat;
    SrtpSession mSrtpSession;
    MediaObserver* mMediaObserver = nullptr;
    void* mMediaObserverTag = nullptr;
  };

  typedef std::shared_ptr<Stream> PStream;

  class StreamList
  {
  public:
    StreamList();
    ~StreamList();

    void add(PStream s);
    void remove(PStream s);
    void clear();
    bool has(PStream s);
    
    int size();
    PStream streamAt(int index);
    
    void copyTo(StreamList* sl);

    Mutex& getMutex();
  protected:
    typedef std::vector<PStream> StreamVector;
    StreamVector mStreamVector;
    Mutex mMutex;
  };
}


#endif
