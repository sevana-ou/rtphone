/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#define NOMINMAX
#include "Audio_DevicePair.h"
#include <algorithm>

#define LOG_SUBSYSTEM "Audio"

using namespace Audio;

// --- DevicePair ---
DevicePair::DevicePair(bool aec, bool agc)
:mConfig(NULL), mDelegate(NULL), mAec(aec), mAgc(agc), mAecFilter(AUDIO_MIC_BUFFER_LENGTH*10, AUDIO_MIC_BUFFER_LENGTH, AUDIO_SAMPLERATE), mAgcFilter(AUDIO_CHANNELS)
{
  mInputBuffer.setCapacity(AUDIO_MIC_BUFFER_SIZE * (AUDIO_MIC_BUFFER_COUNT + 1));
  mOutputBuffer.setCapacity(AUDIO_SPK_BUFFER_SIZE * (AUDIO_SPK_BUFFER_COUNT + 1));
  mInputResampingData.setCapacity(AUDIO_MIC_BUFFER_SIZE * (AUDIO_MIC_BUFFER_COUNT + 1));
  mOutput10msBuffer.setCapacity((int)Format().sizeFromTime(AUDIO_SPK_BUFFER_LENGTH));
  mOutputNativeData.setCapacity((int)Format().sizeFromTime(AUDIO_SPK_BUFFER_LENGTH * AUDIO_SPK_BUFFER_COUNT * 24));
}

DevicePair::~DevicePair()
{
  if (mInput)
  {
    if (mInput->connection() == this)
      mInput->setConnection(NULL);
    mInput.reset();
  }

  if (mOutput)
  {
    if (mOutput->connection() == this)
      mOutput->setConnection(NULL);
    mOutput.reset();
  }
}

VariantMap* DevicePair::config()
{
  return mConfig;
}

void DevicePair::setConfig(VariantMap* config)
{
  mConfig = config;
}

PInputDevice DevicePair::input()
{
  return mInput;
}

void DevicePair::setInput(PInputDevice input)
{
  if (mInput == input)
    return;

  mInput = input;
  mInput->setConnection(this);
  if (mDelegate)
    mDelegate->deviceChanged(this);
}

POutputDevice DevicePair::output()
{
  return mOutput;
}

void DevicePair::setOutput(POutputDevice output)
{
  if (output == mOutput)
    return;
  mOutput = output;  
  mOutput->setConnection(this);
  if (mDelegate)
    mDelegate->deviceChanged(this);
}

bool DevicePair::start()
{
  bool result = false;
  if (mInput)
    result = mInput->open();
  if (mOutput && result)
    result &= mOutput->open();
  return result;
}

void DevicePair::stop()
{
  if (mInput)
    mInput->close();
  if (mOutput)
    mOutput->close();
}

void DevicePair::setDelegate(Delegate* dc)
{
  mDelegate = dc;
}

DevicePair::Delegate* DevicePair::delegate()
{
  return mDelegate;
}

Player& DevicePair::player()
{
  return mPlayer;
}

void DevicePair::onMicData(const Format& f, const void* buffer, int length)
{
#ifdef DUMP_NATIVEINPUT
  if (!mNativeInputDump)
  {
    mNativeInputDump = std::make_shared<WavFileWriter>();
    mNativeInputDump->open("nativeinput.wav", f.mRate, f.mChannels);
  }
  if (mNativeInputDump)
    mNativeInputDump->write(buffer, length);
#endif

  // send the data to internal queue - it can hold data which were not processed by resampler in last call
  mInputResampingData.add(buffer, length);

  // split processing by blocks
  int blocks = mInputResampingData.filled() / (int)f.sizeFromTime(AUDIO_MIC_BUFFER_LENGTH);

  for (int blockIndex = 0; blockIndex < blocks; blockIndex++)
  {

    int wasProcessed = 0;

    int wasProduced = mMicResampler.resample(f.mRate,                                        // Source rate
                                             mInputResampingData.data(),                     // Source data
                                             (int)f.sizeFromTime(AUDIO_MIC_BUFFER_LENGTH),   // Source size
                                             wasProcessed,
                                             AUDIO_SAMPLERATE,                               // Dest rate
                                             mInputBuffer.mutableData() + mInputBuffer.filled(),
                                             mInputBuffer.capacity() - mInputBuffer.filled());

    mInputBuffer.setFilled(mInputBuffer.filled() + wasProduced);
    mInputResampingData.erase((int)f.sizeFromTime(AUDIO_MIC_BUFFER_LENGTH));
    processMicData(Format(), mInputBuffer.mutableData(), (int)Format().sizeFromTime(AUDIO_MIC_BUFFER_LENGTH));

    mInputBuffer.erase((int)Format().sizeFromTime(AUDIO_MIC_BUFFER_LENGTH));
  }

}

void DevicePair::onSpkData(const Format& f, void* buffer, int length)
{
  //ICELogMedia(<< "Audio::DevicePair::onSpkData() begin");
#ifdef DUMP_NATIVEOUTPUT
  if (!mNativeOutputDump)
  {
    mNativeOutputDump = std::make_shared<WavFileWriter>();
    mNativeOutputDump->open("nativeoutput.wav", f.mRate, f.mChannels);
  }
#endif
#ifdef CONSOLE_LOGGING
  printf("Speaker requests %d\n", length);
#endif

  Format nativeFormat = mOutput->getFormat();
  // See how much bytes are needed yet - mOutputNativeData can contain some data already
  int required = length - mOutputNativeData.filled();
  if (required > 0)
  {

    // Find how much blocks must be received from RTP/decoder side
    int nativeBufferSize = (int)nativeFormat.sizeFromTime(AUDIO_SPK_BUFFER_LENGTH);
    int blocks = required / nativeBufferSize;
    if (required % nativeBufferSize)
      blocks++;

    // Now request data from terminal or whetever delegate is
    for (int blockIndex = 0; blockIndex < blocks; blockIndex++)
    {
      memset(mOutput10msBuffer.mutableData(), 0, (size_t)mOutput10msBuffer.capacity());

      if (mDelegate)
        mDelegate->onSpkData(Format(), mOutput10msBuffer.mutableData(), mOutput10msBuffer.capacity());

      // Replace received data with custom file or data playing
      mPlayer.onSpkData(Format(), mOutput10msBuffer.mutableData(), mOutput10msBuffer.capacity());

      // Save it to process with AEC
      if (mAec)
        mAecSpkBuffer.add(mOutput10msBuffer.data(), mOutput10msBuffer.capacity());

      // Resample these 10 milliseconds it to native format
      int wasProcessed = 0;
      int wasProduced = mSpkResampler.resample(AUDIO_SAMPLERATE, mOutput10msBuffer.data(), mOutput10msBuffer.capacity(), wasProcessed, f.mRate,
                                               mOutputNativeData.mutableData() + mOutputNativeData.filled(), mOutputNativeData.capacity() - mOutputNativeData.filled());
      mOutputNativeData.setFilled(mOutputNativeData.filled() + wasProduced);
#ifdef CONSOLE_LOGGING
      printf("Resampled %d to %d\n", wasProcessed, wasProduced);
#endif
    }
  }

  assert(mOutputNativeData.filled() >= length);
#ifdef DUMP_NATIVEOUTPUT
  if (mNativeOutputDump)
    mNativeOutputDump->write(mOutputNativeData.data(), length);
#endif

  mOutputNativeData.read(buffer, length);

  #define AEC_FRAME_SIZE (AUDIO_CHANNELS * (AUDIO_SAMPLERATE / 1000) * AEC_FRAME_TIME * sizeof(short))

  // AEC filter wants frames.
  if (mAec)
  {
    int nrOfFrames = mAecSpkBuffer.filled() / AEC_FRAME_SIZE;
    for (int frameIndex=0; frameIndex < nrOfFrames; frameIndex++)
      mAecFilter.toSpeaker(mAecSpkBuffer.mutableData() + AEC_FRAME_SIZE * frameIndex);
    mAecSpkBuffer.erase(nrOfFrames * AEC_FRAME_SIZE);
  }
  //ICELogMedia(<< "Audio::DevicePair::onSpkData() end")
}

void DevicePair::processMicData(const Format& f, void* buffer, int length)
{
  if (mAgc)
    mAgcFilter.process(buffer, length);

  if (mAec)
    mAecFilter.fromMic(buffer);

  if (mDelegate)
    mDelegate->onMicData(f, buffer, length);
}
