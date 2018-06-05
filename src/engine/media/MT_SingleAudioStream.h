/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_SSRC_STREAM_H
#define __MT_SSRC_STREAM_H

#include "jrtplib/src/rtppacket.h"
#include <map>
#include "MT_Codec.h"
#include "MT_WebRtc.h"
#include "MT_AudioReceiver.h"
namespace MT
{
  class SingleAudioStream
  {
  public:
    SingleAudioStream(const CodecList::Settings& codecSettings, Statistics& stat);
    ~SingleAudioStream();
    void process(std::shared_ptr<jrtplib::RTPPacket> packet);
    void copyPcmTo(Audio::DataWindow& output, int needed);

  protected:
    DtmfReceiver mDtmfReceiver;
    AudioReceiver mReceiver;
  };
  
  typedef std::map<unsigned, SingleAudioStream*> AudioStreamMap; 
}
#endif
