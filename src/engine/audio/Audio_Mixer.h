/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _RX_MIXER_H
#define _RX_MIXER_H

#include "../engine_config.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Sync.h"
#include "Audio_Resampler.h"
#include "Audio_DataWindow.h"
#include <map>
#include <atomic>

namespace Audio 
{
  class Mixer
  {
  protected:
    class Stream
    {
    protected:
      DataWindow                mData;
      Resampler                 mResampler8, 
                                mResampler16, 
                                mResampler32,
                                mResampler48;
      bool                      mActive;
      void*                     mContext;
      unsigned                  mSSRC;
		  unsigned                  mFadeOutCounter;
      ByteBuffer                mTempBuffer;

    public:
      Stream();
      ~Stream();

      void setSsrc(unsigned ssrc);
      unsigned ssrc();
      void setContext(void* context);
      void* context();
      DataWindow& data();
      bool active();
      void setActive(bool active);
      void addPcm(int rate, const void* input, int length);
    };
  
    Stream                            mChannelList[AUDIO_MIX_CHANNEL_COUNT];
	  Mutex												      mMutex;
    DataWindow                        mOutput;
    std::atomic_int                   mActiveCounter;

    void mix();
    Stream* allocateChannel(void* context, unsigned ssrc);

  public:
    Mixer();
    ~Mixer();
  
    void unregisterChannel(void* context);
    void clear(void* context, unsigned ssrc);
    void addPcm(void* context, unsigned ssrc, const void* inputData, int inputLength, int inputRate, bool fadeOut);
    void addPcm(void* context, unsigned ssrc, Audio::DataWindow& w, int rate, bool fadeOut);
    int  getPcm(void* outputData, int outputLength);
    int  mixAndGetPcm(Audio::DataWindow& output);
	  int  available();
  };
} //end of namespace

#endif
