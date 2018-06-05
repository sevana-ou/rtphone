/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../config.h"
#include "Audio_Quality.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Types.h"
#include "speex/speex_preprocess.h"

#ifdef WIN32
# include <malloc.h>
#endif
#include <assert.h>
#include <string.h>

using namespace Audio;

#define SHRT_MAX      32767         /* maximum (signed) short value */
AgcFilter::AgcFilter(int channels)
{
  static const float DefaultLevel = 0.8f;

  for (int i=0; i<channels; i++)
  {
    Channel c;
    float level = DefaultLevel;
    c.mSampleMax = 1;
    c.mCounter = 0;
    c.mIgain = 65536;
    if (level > 1.0f)
      level = 1.0f;
    else
    if (level < 0.5f)
      level = 0.5f;

    c.mIpeak = (int)(SHRT_MAX * level * 65536);

    c.mSilenceCounter = 0;
    mChannelList.push_back(c);
  }
}

AgcFilter::~AgcFilter()
{
}

void AgcFilter::process(void *pcm, int length)
{
  for (size_t i=0; i<mChannelList.size(); i++)
    processChannel((short*)pcm, length / (sizeof(short) * mChannelList.size()), i);
}

void AgcFilter::processChannel(short* pcm, int nrOfSamples, int channelIndex)
{
  int i;

  for(i=0; i<nrOfSamples; i++)
  {
    long gain_new;
    int sample;
    int sampleIndex = mChannelList.size() * i + channelIndex;
    Channel& channel = mChannelList[channelIndex];

    /* get the abs of buffer[i] */
    sample = pcm[sampleIndex];
    sample = (sample < 0 ? -(sample):sample);

    if(sample > (int)channel.mSampleMax)
    {
      /* update the max */
      channel.mSampleMax = (unsigned int)sample;
    }
    channel.mCounter ++;

    /* Will we get an overflow with the current gain factor? */
    if (((sample * channel.mIgain) >> 16) > channel.mIpeak)
    {
      /* Yes: Calculate new gain. */
      channel.mIgain = ((channel.mIpeak / channel.mSampleMax) * 62259) >> 16;
      channel.mSilenceCounter = 0;
      pcm[sampleIndex] = (short) ((pcm[sampleIndex] * channel.mIgain) >> 16);
      continue;
    }

    /* Calculate new gain factor 10x per second */
    if (channel.mCounter >= AUDIO_SAMPLERATE / 10)
    {
      if (channel.mSampleMax > AUDIO_SAMPLERATE / 10)        /* speaking? */
      {
        gain_new = ((channel.mIpeak / channel.mSampleMax) * 62259) >> 16;
                
        if (channel.mSilenceCounter > 40)  /* pause -> speaking */
          channel.mIgain += (gain_new - channel.mIgain) >> 2;
        else
          channel.mIgain += (gain_new - channel.mIgain) / 20;

        channel.mSilenceCounter = 0;
      }
      else   /* silence */
      {
        channel.mSilenceCounter++;
        /* silence > 2 seconds: reduce gain */
        if ((channel.mIgain > 65536) && (channel.mSilenceCounter >= 20))
          channel.mIgain = (channel.mIgain * 62259) >> 16;
      }
      
      channel.mCounter = 0;
      channel.mSampleMax = 1;
    }
    pcm[sampleIndex] = (short) ((pcm[sampleIndex] * channel.mIgain) >> 16);
  }
}

// --- AecFilter ---
#ifdef USE_SPEEX_AEC
# include "speex/speex_echo.h"
#include "Audio_Interface.h"

#if !defined(TARGET_WIN)
# include <alloca.h>
#endif

#endif

#ifdef USE_WEBRTC_AEC
# include "aec/echo_cancellation.h"
#endif
 
#ifdef USE_WEBRTC_AEC
static void CheckWRACode(unsigned errorcode)
{
  if (errorcode)
    throw Exception(ERR_WEBRTC, errorcode);
}
#endif

AecFilter::AecFilter(int tailTime, int frameTime, int rate)
:mCtx(nullptr), mFrameTime(frameTime), mRate(rate)
{
#ifdef USE_SPEEX_AEC
  if (AUDIO_CHANNELS == 2)
    mCtx = speex_echo_state_init_mc(frameTime * (mRate / 1000), tailTime * (mRate / 1000), AUDIO_CHANNELS, AUDIO_CHANNELS );
  else
    mCtx = speex_echo_state_init(frameTime * (mRate / 1000), tailTime * (mRate / 1000));
  int tmp = rate;
  speex_echo_ctl((SpeexEchoState*)mCtx, SPEEX_ECHO_SET_SAMPLING_RATE, &tmp);
#endif

#ifdef USE_WEBRTC_AEC
  CheckWRACode(WebRtcAec_Create(&mCtx));
  CheckWRACode(WebRtcAec_Init(mCtx, rate, rate));
#endif
}

AecFilter::~AecFilter()
{
#ifdef USE_SPEEX_AEC
  if (mCtx)
  {
    //speex_echo_state_destroy((SpeexEchoState*)mCtx);
    mCtx = nullptr;
  }
#endif

#ifdef USE_WEBRTC_AEC
  CheckWRACode(WebRtcAec_Free(mCtx));
  mCtx = NULL;
#endif
}

void AecFilter::fromMic(void *data)
{
#ifdef USE_SPEEX_AEC
  short* output = (short*)alloca(Format().sizeFromTime(AUDIO_MIC_BUFFER_LENGTH));
  speex_echo_capture((SpeexEchoState*)mCtx, (short*)data, (short*)output);
  memmove(data, output, AUDIO_MIC_BUFFER_SIZE);
#endif

#ifdef USE_WEBRTC_AEC
    short* inputframe = (short*)ALLOCA(framesize);
    memcpy(inputframe, (char*)data+framesize*i, framesize);
    CheckWRACode(WebRtcAec_Process(mCtx, (short*)inputframe, NULL, (short*)data+framesize/2*i, NULL, mFrameTime * mRate / 1000, 0,0));
#endif
}

void AecFilter::toSpeaker(void *data)
{
#ifdef USE_SPEEX_AEC
  speex_echo_playback((SpeexEchoState*)mCtx, (short*)data);
#endif

#ifdef USE_WEBRTC_AEC
  CheckWRACode(WebRtcAec_BufferFarend(mCtx, (short*)data, length / 2 / AUDIO_CHANNELS));
#endif
}

int AecFilter::frametime()
{
  return mFrameTime;
}


DenoiseFilter::DenoiseFilter(int rate)
:mRate(rate)
{
  mCtx = speex_preprocess_state_init(mRate/100, mRate);
}

DenoiseFilter::~DenoiseFilter()
{
  if (mCtx)
    speex_preprocess_state_destroy((SpeexPreprocessState*)mCtx);
}

void DenoiseFilter::fromMic(void* data, int timelength)
{
  assert(timelength % 10 == 0);
  
  // Process by 10-ms blocks
  spx_int16_t* in  = (spx_int16_t*)data;
  
  for (int blockIndex=0; blockIndex<timelength/10; blockIndex++)
  {
    spx_int16_t* block = in + blockIndex * (mRate / 100) * AUDIO_CHANNELS;
    speex_preprocess_run((SpeexPreprocessState*)mCtx, block);
  }
}

int DenoiseFilter::rate()
{
  return mRate;
}

