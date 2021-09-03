#include "Audio_AndroidOboe.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_Log.h"
#include <mutex>
#include <iostream>
#include <stdexcept>

#include "../helper/HL_String.h"
#include "../helper/HL_Time.h"

#ifdef TARGET_ANDROID

#define LOG_SUBSYSTEM "Audio"

using namespace Audio;


// -------------------- AndroidEnumerator -----------------------------

AndroidEnumerator::AndroidEnumerator()
{}

AndroidEnumerator::~AndroidEnumerator()
{}

int AndroidEnumerator::indexOfDefaultDevice()
{
  return 0;
}

int AndroidEnumerator::count()
{
  return 1;
}

int AndroidEnumerator::idAt(int index)
{
  return 0;
}

std::string AndroidEnumerator::nameAt(int index)
{
  return "Audio";
}

void AndroidEnumerator::open(int direction)
{}

void AndroidEnumerator::close()
{}

// --------------- Input implementation ----------------
AndroidInputDevice::AndroidInputDevice(int devId)
{}

AndroidInputDevice::~AndroidInputDevice()
{
  close();
}

bool AndroidInputDevice::open()
{
  if (active())
    return true;

  oboe::AudioStreamBuilder builder;
  builder.setDirection(oboe::Direction::Input);
  builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  builder.setSharingMode(oboe::SharingMode::Exclusive);
  builder.setFormat(oboe::AudioFormat::I16);
  builder.setChannelCount(oboe::ChannelCount::Mono);
  builder.setCallback(this);
  oboe::Result rescode = builder.openStream(&mRecordingStream);
  if (rescode != oboe::Result::OK)
    return false;

  mDeviceRate = mRecordingStream->getSampleRate();
  ICELogInfo(<< "Input Opened with rate " << mDeviceRate);
  mActive = true;

  rescode = mRecordingStream->requestStart();
  if (rescode != oboe::Result::OK)
  {
      close();
      mActive = false;
  }
  return mActive;
}

void AndroidInputDevice::close()
{
  // There is no check for active() value because close() can be called to cleanup after bad open() call.
  if (mRecordingStream != nullptr)
  {
    mRecordingStream->close();
    delete mRecordingStream; mRecordingStream = nullptr;
  }
  mActive = false;
}

oboe::DataCallbackResult
AndroidInputDevice::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames)
{
  std::unique_lock<std::mutex> l(mMutex);

  // Send data to AudioPair
  if (mConnection)
    mConnection->onMicData(getFormat(), audioData, numFrames);

  return oboe::DataCallbackResult::Continue;
}

Format AndroidInputDevice::getFormat()
{
  return Format(mDeviceRate, 1);
}

bool AndroidInputDevice::active() const
{
  return mActive;
}

bool AndroidInputDevice::fakeMode()
{
  return false;
}

void AndroidInputDevice::setFakeMode(bool fakemode)
{}

int AndroidInputDevice::readBuffer(void* buffer)
{
  throw std::runtime_error("AndroidInputDevice::readBuffer() is not implemented.");
}

// ------------ AndroidOutputDevice -----------------
AndroidOutputDevice::AndroidOutputDevice(int devId)
{
  ICELogDebug(<< "Creating AndroidOutputDevice. This is: " << StringHelper::toHex(this));
}
AndroidOutputDevice::~AndroidOutputDevice()
{
  ICELogDebug(<< "Deleting AndroidOutputDevice.");
  close();
}

bool AndroidOutputDevice::open()
{
  std::unique_lock<std::mutex> l(mMutex);

  if (mActive)
    return true;

  mRequestedFrames = 0;
  mStartTime = 0.0;
  mEndTime = 0.0;

  oboe::AudioStreamBuilder builder;
  builder.setDirection(oboe::Direction::Output);
  builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  builder.setSharingMode(oboe::SharingMode::Exclusive);
  builder.setFormat(oboe::AudioFormat::I16);
  builder.setChannelCount(oboe::ChannelCount::Mono);
  // builder.setDataCallback(this);
  builder.setCallback(this);
  //builder.setErrorCallback(this)

  oboe::Result rescode = builder.openStream(&mPlayingStream);
  if (rescode != oboe::Result::OK)
    return false;

  mDeviceRate = mPlayingStream->getSampleRate();
  ICELogInfo(<< "Input Opened with rate " << mDeviceRate);
  mActive = true;

  rescode = mPlayingStream->requestStart();
  if (rescode != oboe::Result::OK)
  {
      close();
      mActive = false;
  }
  return mActive;
}

void AndroidOutputDevice::close()
{
  std::unique_lock<std::mutex> l(mMutex);
  if (!mActive)
    return;

  if (mPlayingStream != nullptr)
  {
    mPlayingStream->close();
    delete mPlayingStream; mPlayingStream = nullptr;
  }
  mEndTime = now_ms();
  mActive = false;

  ICELogInfo(<< "For time " << mEndTime - mStartTime << " ms was requested "
             << float(mRequestedFrames) / getFormat().mRate * 1000 << " ms");
}

Format AndroidOutputDevice::getFormat()
{
  return Format(mDeviceRate, 1);
}

bool AndroidOutputDevice::fakeMode()
{
  return false;
}

void AndroidOutputDevice::setFakeMode(bool /*fakemode*/)
{
}

oboe::DataCallbackResult AndroidOutputDevice::onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames)
{
  if (mInShutdown)
    return oboe::DataCallbackResult::Stop;

  if (mStartTime == 0.0)
    mStartTime = now_ms();

  // Ask producer about data
  memset(audioData, 0, numFrames * 2);
  if (mConnection)
  {
    Format f = getFormat();
    if (f.mRate != 0)
      mConnection->onSpkData(f, audioData, numFrames * 2);
  }
  mRequestedFrames += numFrames;

  return oboe::DataCallbackResult::Continue;
}

// TODO - special case https://github.com/google/oboe/blob/master/docs/notes/disconnect.md
void AndroidOutputDevice::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result) {
  if (result == oboe::Result::ErrorDisconnected) {
    // LOGI("Restarting AudioStream after disconnect");
    // soundEngine.restart(); // please check oboe samples for soundEngine.restart(); call
  }
}
#endif // TARGET_ANDROID