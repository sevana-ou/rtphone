/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_QUALITY_H
#define __AUDIO_QUALITY_H
#include "../config.h"
#include "../helper/HL_Sync.h"
#include <vector>

namespace Audio
{
  class AgcFilter
  {
  protected:
    struct Channel
    {
      unsigned int    mSampleMax;
      int             mCounter;
      long            mIgain;
      int             mIpeak;
      int             mSilenceCounter;
    };
    std::vector<Channel> mChannelList;
    void processChannel(short* pcm, int nrOfSamples, int channelIndex);
  public:
    AgcFilter(int channels);
    ~AgcFilter();
    
    void  process(void* pcm, int length);
  };

  class AecFilter
  {
  public:
    AecFilter(int tailTime, int frameTime, int rate);
    ~AecFilter();

    // These methods accept input block with timelength "frameTime" used in constructor.
    void toSpeaker(void* data);
    void fromMic(void* data);
    int frametime();
  
  protected:
    void*               mCtx;             /// The echo canceller context's pointer.
    Mutex               mGuard;           /// Mutex to protect this instance.
    int                 mFrameTime;         /// Duration of single audio frame (in milliseconds)
    int                 mRate;
  };

  class DenoiseFilter
  {
  public:
    DenoiseFilter(int rate);
    ~DenoiseFilter();

    void fromMic(void* data, int timelength);
    int rate();

  protected:
    Mutex               mGuard;           /// Mutex to protect this instance.
    void*               mCtx;             /// The denoiser context pointer.
    int                 mRate;            /// Duration of single audio frame (in milliseconds)
  };

}

#endif
