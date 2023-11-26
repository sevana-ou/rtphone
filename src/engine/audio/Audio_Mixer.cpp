/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "../engine_config.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Log.h"

#include <algorithm>
#include <assert.h>

#include "Audio_Mixer.h"

#define LOG_SUBSYSTEM "Mixer"
using namespace Audio;

Mixer::Stream::Stream()
{
  mResampler8.start(AUDIO_CHANNELS, 8000, AUDIO_SAMPLERATE);
  mResampler16.start(AUDIO_CHANNELS, 16000, AUDIO_SAMPLERATE);
  mResampler32.start(AUDIO_CHANNELS, 32000, AUDIO_SAMPLERATE);
  mResampler48.start(AUDIO_CHANNELS, 48000, AUDIO_SAMPLERATE);
  mActive = false;
  mContext = nullptr;
  mSSRC = 0;
  mFadeOutCounter = 0;
  mData.setCapacity(AUDIO_SPK_BUFFER_SIZE * AUDIO_SPK_BUFFER_COUNT);
}

Mixer::Stream::~Stream()
{
}

void Mixer::Stream::setSsrc(unsigned ssrc)
{
  mSSRC = ssrc;
}

unsigned Mixer::Stream::ssrc()
{
  return mSSRC;
}

void Mixer::Stream::setContext(void* context)
{
  mContext = context;
}
void* Mixer::Stream::context()
{
  return mContext;
}

DataWindow& Mixer::Stream::data()
{
  return mData;
}

bool Mixer::Stream::active()
{
  return mActive;
}

void Mixer::Stream::setActive(bool active)
{
  mActive = active;
}

void Mixer::Stream::addPcm(int rate, const void* input, int length)
{
  // Resample to internal sample rate
  size_t outputSize = size_t(0.5 + length * ((float)AUDIO_SAMPLERATE / rate));
  if (mTempBuffer.size() < outputSize)
    mTempBuffer.resize(outputSize);
  
  Resampler* resampler = (rate == 8000) ? &mResampler8 : ((rate == 16000) ? &mResampler16 : ((rate == 32000) ? &mResampler32 : &mResampler48));
  size_t inputProcessed = 0;
  resampler->processBuffer(input, length, inputProcessed, mTempBuffer.mutableData(), outputSize);
  // inputProcessed result value is ignored here - rate will be 8/16/32/48k, inputProcessed is equal to length

  // Queue data
  mData.add(mTempBuffer.data(), outputSize);
}

Mixer::Mixer()
{
  mActiveCounter = 0;
  mOutput.setCapacity(32768);
}

Mixer::~Mixer()
{
}

void Mixer::unregisterChannel(void* channel)
{
  for (int i=0; i<AUDIO_MIX_CHANNEL_COUNT; i++)
  {
    Stream& c = mChannelList[i];
    if (c.active() && c.context() == channel)
    {
       c.setActive(false);  // stream is not active anymore
       c.data().clear();    // clear data
       mActiveCounter--;
    }
  }
}

void Mixer::clear(void* context, unsigned ssrc)
{
  for (int i=0; i<AUDIO_MIX_CHANNEL_COUNT; i++)
  {
    Stream& c = mChannelList[i];
    if (c.active() && c.context() == context && c.ssrc() == ssrc)
    {
       c.setActive(false);
       c.data().clear();
       mActiveCounter--;
    }
  }
}

Mixer::Stream* Mixer::allocateChannel(void* context, unsigned ssrc)
{
  // Allocate new channel
  Lock l(mMutex);
  Stream* channel;
  for (int i=0; i<AUDIO_MIX_CHANNEL_COUNT;i++)
  {
    channel = &mChannelList[i];
    if (!channel->active())
    {
      channel->setSsrc(ssrc);
      channel->setContext(context);
      channel->data().clear();
      mActiveCounter++;
      channel->setActive(true);
      return channel;
    }
  }
  return NULL;
}

void Mixer::addPcm(void* context, unsigned ssrc, 
                         const void* inputData, int inputLength, 
                         int inputRate, bool fadeOut)
{
  assert(inputRate == 8000 || inputRate == 16000 || inputRate == 32000);

  int i;

  // Locate a channel
  Stream* channel = NULL;
  
  for (i=0; i<AUDIO_MIX_CHANNEL_COUNT && !channel; i++)
  {
    Stream& c = mChannelList[i];
    if (c.active() && c.context() == context && c.ssrc() == ssrc)
      channel = &c;
  }
  if (!channel)
  {
    channel = allocateChannel(context, ssrc);
    if (!channel)
      throw Exception(ERR_MIXER_OVERFLOW);
  }
  
  channel->addPcm(inputRate, inputData, inputLength);
}

void Mixer::addPcm(void* context, unsigned ssrc, Audio::DataWindow& w, int rate, bool fadeOut)
{
  assert(rate == 8000 || rate == 16000 || rate == 32000 || rate == 48000);

  int i;

  // Locate a channel
  Stream* channel = NULL;
  
  for (i=0; i<AUDIO_MIX_CHANNEL_COUNT && !channel; i++)
  {
    Stream& c = mChannelList[i];
    if (c.active() && c.context() == context && c.ssrc() == ssrc)
      channel = &c;
  }
  if (!channel)
  {
    channel = allocateChannel(context, ssrc);
    if (!channel)
      throw Exception(ERR_MIXER_OVERFLOW);
  }
  
  channel->addPcm(rate, w.data(), w.filled());
  //ICELogSpecial(<<"Mixer stream " << int(this) << " has " << w.filled() << " bytes");
}

void Mixer::mix()
{
  // Current sample
  int sample = 0;

  // Counter of processed active channels
  int processed = 0;

  // Samples & sources counters
  unsigned sampleCounter = 0, sourceCounter;

  short outputBuffer[512];
  unsigned outputCounter = 0;

	// Build active channel map
	Stream* channelList[AUDIO_MIX_CHANNEL_COUNT];
	int activeCounter = 0;
	for (int i=0; i<AUDIO_MIX_CHANNEL_COUNT; i++)
		if (mChannelList[i].active())
			channelList[activeCounter++] = &mChannelList[i];
	
  // No active channels - nothing to mix - exit
  if (!activeCounter)
  {
    // ICELogDebug(<< "No active channel");
    return;
  }
	
	// Optimized versions for 1&  2  active channels
  if (activeCounter == 1)
  {
    // Copy much samples as we have
    Stream& audio = *channelList[0];

    // Copy the decoded data
    mOutput.add(audio.data().data(), audio.data().filled());

	// Erase copied audio samples
	audio.data().erase(audio.data().filled());
    //ICELogSpecial(<<"Length of mixer stream " << audio.data().filled());
  }
  else	
  if (activeCounter == 2)
  {
    Stream& audio1 = *channelList[0];
	Stream& audio2 = *channelList[1];
	int filled1 = audio1.data().filled() / 2, filled2 = audio2.data().filled() / 2;
    int available = filled1 > filled2 ? filled1 : filled2;
		
	// Find how much samples can be mixed
	int filled = mOutput.filled() / 2;
		
    int maxsize = mOutput.capacity() / 2;
	if (maxsize - filled < available)
      available = maxsize - filled;
		
	short sample = 0;
	for (int i=0; i<available; i++)
	{
      short sample1 = filled1 > i ? audio1.data().shortAt(i) : 0;
      short sample2 = filled2 > i ? audio2.data().shortAt(i) : 0;
      sample = (abs(sample1) > abs(sample2)) ? sample1 : sample2;

      mOutput.add(sample);
	}
    audio1.data().erase(available*2);
	audio2.data().erase(available*2);
  }
  else
  {
    do
    {
      sample = 0;
      sourceCounter = 0;
      processed = 0;
      for (int i=0; i<activeCounter; i++)
      {
        Stream& audio = *channelList[i];
        processed++;

        if (audio.data().filled() > (int)sampleCounter * 2)
        {
          short currentSample = audio.data().shortAt(sampleCounter);
          if (abs(currentSample) > abs(sample))
            sample = currentSample;
          sourceCounter++;
        }
      }

      if (sourceCounter)
      {
        outputBuffer[outputCounter++] = (short)sample;
        sampleCounter++;
      }

      // Check if time to flash output buffer
      if ((!sourceCounter || outputCounter == 512) && outputCounter)
      {
        mOutput.add(outputBuffer, outputCounter * 2);
        outputCounter = 0;
      }
    }
    while (sourceCounter);
    
    processed = 0;
    for (int i=0; i<activeCounter; i++)
    {
      Stream& audio = *channelList[i];
      audio.data().erase(sampleCounter*2);
    }
  }
}


int Mixer::getPcm(void* outputData, int outputLength)
{
  if (mOutput.filled() < outputLength)
    mix();
      
    //ICELogSpecial(<<"Mixer has " << mOutput.filled() << " available bytes");
  memset(outputData, 0, outputLength);
  return mOutput.read(outputData, outputLength);
}

int Mixer::mixAndGetPcm(Audio::DataWindow& output)
{
  // Mix
  mix();

  // Set output space
  output.setCapacity(mOutput.filled());

  // Read mixed data to output
  return mOutput.read(output.mutableData(), output.capacity());
}

int Mixer::available()
{
  return mOutput.filled();
}

