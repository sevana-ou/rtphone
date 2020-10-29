/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __MT_BOX_H
#define __MT_BOX_H

#include <set>
#include "MT_Stream.h"
#include "MT_CodecList.h"
#include "../audio/Audio_Interface.h"
#include "../audio/Audio_DevicePair.h"
#include "../audio/Audio_Mixer.h"
#include "../helper/HL_VariantMap.h"

namespace MT
{

  class Terminal: public Audio::DevicePair::Delegate
  {
  public:
    Terminal(const CodecList::Settings& codecSettings);
    virtual ~Terminal();
    
    CodecList& codeclist();  

    PStream createStream(int type, VariantMap& config);
    void freeStream(PStream s);
    
    Audio::PDevicePair audio();
    void setAudio(const Audio::PDevicePair& audio);
  protected:
    StreamList mAudioList;
    std::mutex mAudioListMutex;
    CodecList mCodecList;
    Audio::PDevicePair mAudioPair;
    Audio::Mixer mAudioMixer;
    Audio::DataWindow mCapturedAudio;

    void deviceChanged(Audio::DevicePair* dp);
    void onMicData(const Audio::Format& f, const void* buffer, int length);
    void onSpkData(const Audio::Format& f, void* buffer, int length);
  };

}
#endif
