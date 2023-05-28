/* Copyright(C) 2007-2023 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Agent_AudioManager.h"
#include "../engine/audio/Audio_WavFile.h"
#include "../engine/audio/Audio_Null.h"

#if defined(TARGET_ANDROID)
# include "../engine/audio/Audio_Android.h"
#endif

#define LOG_SUBSYSTEM "AudioManager"


AudioManager::AudioManager()
:mTerminal(nullptr), mAudioMonitoring(nullptr)
{
  mPlayer.setDelegate(this);
}

AudioManager::~AudioManager()
{
  // stop();
}

AudioManager& AudioManager::instance()
{
  static std::shared_ptr<AudioManager> GAudioManager;
  
  if (!GAudioManager)
    GAudioManager = std::make_shared<AudioManager>();
  
  return *GAudioManager;
}

void AudioManager::setTerminal(MT::Terminal* terminal)
{
  mTerminal = terminal;
}

MT::Terminal* AudioManager::terminal()
{
  return mTerminal;
}

void AudioManager::setAudioMonitoring(Audio::DataConnection* monitoring)
{
  mAudioMonitoring = monitoring;
}

Audio::DataConnection* AudioManager::audioMonitoring()
{
  return mAudioMonitoring;
}

#define LOCK_MANAGER std::unique_lock<std::mutex> l(mGuard)
void AudioManager::start(int usageId)
{
  assert(mTerminal);
  LOCK_MANAGER;
  
  ICELogInfo(<< "Start main audio with usage id " << usageId);

  if (mUsage.obtain(usageId) > 1)
    return;

  if (Audio::OsEngine::instance())
    Audio::OsEngine::instance()->open();

  if (!mAudioInput || !mAudioOutput)
  {
      // Disable AEC for now - because PVQA conflicts with speex AEC.
      std::shared_ptr<Audio::Enumerator> enumerator(Audio::Enumerator::make(usageId == atNull));
      if (!mTerminal->audio())
      {
          auto audio = std::make_shared<Audio::DevicePair>();
          audio->setAgc(true);
          audio->setAec(false);
          audio->setMonitoring(mAudioMonitoring);

          mTerminal->setAudio(audio);
      }

      if (!mAudioInput)
      {
          enumerator->open(Audio::myMicrophone);
          int inputIndex = enumerator->indexOfDefaultDevice();

          // Construct and set to terminal's audio pair input device
          if (usageId != atNull)
              mAudioInput = Audio::PInputDevice(Audio::InputDevice::make(enumerator->idAt(inputIndex)));
          else
              mAudioInput = Audio::PInputDevice(new Audio::NullInputDevice());

          mTerminal->audio()->setInput(mAudioInput);
      }

      if (!mAudioOutput)
      {
          Audio::Enumerator *enumerator = Audio::Enumerator::make(usageId == atNull);
          enumerator->open(Audio::mySpeaker);
          int outputIndex = enumerator->indexOfDefaultDevice();

          // Construct and set terminal's audio pair output device
          if (usageId != atNull)
          {
            if (outputIndex >= enumerator->count())
              outputIndex = 0;

            mAudioOutput = Audio::POutputDevice(
                  Audio::OutputDevice::make(enumerator->idAt(outputIndex)));
          }
          else
              mAudioOutput = Audio::POutputDevice(new Audio::NullOutputDevice());

          mTerminal->audio()->setOutput(mAudioOutput);
      }
  }

  // Open audio
  if (mAudioInput)
    mAudioInput->open();
  if (mAudioOutput)
    mAudioOutput->open();
}

void AudioManager::close()
{
  mUsage.clear();
  if (mAudioInput)
  {
    mAudioInput->close();
    mAudioInput.reset();
  }

  if (mAudioOutput)
  {
    mAudioOutput->close();
    mAudioOutput.reset();
  }
  mPlayer.setOutput(Audio::POutputDevice());
}

void AudioManager::stop(int usageId)
{
  LOCK_MANAGER;
  
  ICELogInfo( << "Stop main audio with usage id " << usageId);
  if (mTerminal)
  {
    if (mTerminal->audio())
      mTerminal->audio()->player().release(usageId);
  }

  if (!mUsage.release(usageId))
  {
    close();

    // Reset device pair on terminal side
    mTerminal->setAudio(Audio::PDevicePair());

    if (Audio::OsEngine::instance())
      Audio::OsEngine::instance()->close();
  }
}

void AudioManager::startPlayFile(int usageId, const std::string& path, AudioTarget target, LoopMode lm, int timelimit)
{
  // Check if file exists
  Audio::PWavFileReader r = std::make_shared<Audio::WavFileReader>();
#ifdef TARGET_WIN
  r->open(StringHelper::makeTstring(path));
#else
  r->open(path);
#endif
  if (!r->isOpened())
  {
    ICELogError(<< "Cannot open file to play");
    return;
  }

  // Delegate processing to existing audio device pair manager
  mTerminal->audio()->player().add(usageId, r, lm == lmLoopAudio, timelimit);
  start(usageId);
}

void AudioManager::stopPlayFile(int usageId)
{
  stop(usageId);
  mPlayer.release(usageId);
}

void AudioManager::onFilePlayed(Audio::Player::PlaylistItem& item)
{
}

void AudioManager::process()
{
  mPlayer.releasePlayed();
  std::vector<int> ids;
  mTerminal->audio()->player().retrieveUsageIds(ids);
  for (unsigned i=0; i<ids.size(); i++)
    stop(ids[i]);
}
