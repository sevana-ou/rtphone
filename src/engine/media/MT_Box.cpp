/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "MT_Box.h"
#include "MT_AudioStream.h"
#include "MT_SrtpHelper.h"

#include "../helper/HL_VariantMap.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_StreamState.h"
#include "../helper/HL_Log.h"

#define LOG_SUBSYSTEM "MT::Box"

using namespace MT;

Terminal::Terminal(const CodecList::Settings& settings)
  :mCodecList(settings)
{
  ICELogDebug(<< "Opening Terminal instance");

  // Alloc memory for captured audio
  mCapturedAudio.setCapacity(AUDIO_MIC_BUFFER_COUNT * AUDIO_MIC_BUFFER_SIZE);

  // Init srtp library
  SrtpSession::initSrtp();
}

Terminal::~Terminal()
{
  ICELogDebug(<< "Closing Terminal instance");
  mAudioPair.reset();
}

PStream Terminal::createStream(int type, VariantMap& config)
{
  PStream result;
  switch (type)
  {
  case Stream::Audio:
    result = PStream(new AudioStream(MT::CodecList::Settings::DefaultSettings));
    mAudioList.add(result);
    break;

  case Stream::Video:
  default:
    throw Exception(ERR_NOT_IMPLEMENTED);
  }

  return result;
}

void Terminal::freeStream(PStream stream)
{
  if (AudioStream* audio = dynamic_cast<AudioStream*>(stream.get()))
  {
    ICELogDebug(<< "Unregister audio stream from mixer.");
    mAudioMixer.unregisterChannel(audio);

    ICELogDebug(<< "Remove audio stream from list.");
    mAudioList.remove(stream);
  }
}

CodecList& Terminal::codeclist()
{
  return mCodecList;
}

Audio::PDevicePair Terminal::audio()
{
  return mAudioPair;
}

void Terminal::setAudio(const Audio::PDevicePair& audio)
{
  mAudioPair = audio;
  if (mAudioPair)
    mAudioPair->setDelegate(this);
}

void Terminal::deviceChanged(Audio::DevicePair* dp)
{
}

void Terminal::onMicData(const Audio::Format& f, const void* buffer, int length)
{
  //ICELogMedia(<< "Got " << length << " bytes from microphone");

  // See if it is enough data to feed streams
  mCapturedAudio.add(buffer, length);
  if (mCapturedAudio.filled() < AUDIO_MIC_BUFFER_SIZE)
      return;

  StreamList sl;
  mAudioList.copyTo(&sl);

  // Iterate streams. See what of them requires microphone data.
  for (int frameIndex=0; frameIndex < mCapturedAudio.filled() / AUDIO_MIC_BUFFER_SIZE; frameIndex++)
  {
      for (int i=0; i<sl.size(); i++)
      {
        if (AudioStream* stream = dynamic_cast<AudioStream*>(sl.streamAt(i).get()))
        {
          if (stream->state() & (int)StreamState::Sending)
            stream->addData(mCapturedAudio.data(), AUDIO_MIC_BUFFER_SIZE);
        }
      }
      mCapturedAudio.erase(AUDIO_MIC_BUFFER_SIZE);
  }
}

void Terminal::onSpkData(const Audio::Format& f, void* buffer, int length)
{
  ICELogMedia(<< "Speaker requests " << length << " bytes");
  if (mAudioMixer.available() < length)
  {
    Lock l(mAudioList.getMutex());

    for (int i=0; i<mAudioList.size(); i++)
    {
      if (AudioStream* stream = dynamic_cast<AudioStream*>(mAudioList.streamAt(i).get()))
      {
        //if (stream->state() & STATE_RECEIVING)
          stream->copyDataTo(mAudioMixer, length - mAudioMixer.available());
      }
    }
  }
  mAudioMixer.getPcm(buffer, length);
}
