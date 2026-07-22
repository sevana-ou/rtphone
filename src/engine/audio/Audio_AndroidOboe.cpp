#include "Audio_AndroidOboe.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_Log.h"
#include <mutex>
#include <iostream>
#include <stdexcept>
#include <algorithm>

#include "../helper/HL_String.h"
#include "../helper/HL_Time.h"

#ifdef TARGET_ANDROID

#define LOG_SUBSYSTEM "audio"

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
  ICELogDebug(<< "Creating AndroidOutputDevice. This is: " << strx::toHex(this));
}
AndroidOutputDevice::~AndroidOutputDevice()
{
  ICELogDebug(<< "Deleting AndroidOutputDevice.");
  // Mark shutdown before closing so a disconnect callback racing with teardown
  // does not resurrect the stream via onErrorAfterClose()'s restart.
  mInShutdown = true;
  close();
}

bool AndroidOutputDevice::open()
{
  std::unique_lock<std::mutex> l(mMutex);

  if (mActive)
    return true;

  mInShutdown = false;
  mRequestedFrames = 0;
  mStartTime = 0.0;
  mEndTime = 0.0;

  oboe::AudioStreamBuilder builder;
  builder.setDirection(oboe::Direction::Output);
  builder.setPerformanceMode(oboe::PerformanceMode::LowLatency);
  builder.setSharingMode(oboe::SharingMode::Exclusive);
  builder.setFormat(oboe::AudioFormat::I16);
  builder.setChannelCount(oboe::ChannelCount::Mono);
  // Route through the platform voice-call path: correct device selection/volume
  // and platform voice tuning for a softphone.
  builder.setUsage(oboe::Usage::VoiceCommunication);
  builder.setContentType(oboe::ContentType::Speech);
  // builder.setDataCallback(this);
  builder.setCallback(this);
  //builder.setErrorCallback(this)

  oboe::Result rescode = builder.openStream(&mPlayingStream);
  if (rescode != oboe::Result::OK)
    return false;

  mDeviceRate = mPlayingStream->getSampleRate();
  ICELogInfo(<< "Output opened with rate " << mDeviceRate);
  mActive = true;

  rescode = mPlayingStream->requestStart();
  if (rescode != oboe::Result::OK)
  {
      close();
      mActive = false;
      return mActive;
  }

  // Latch the burst size and start from a two-burst buffer. onAudioReady() grows
  // this on XRuns (up to a cap) so we keep low latency when the device can sustain
  // it and trade a little latency for glitch-free playback when it can't.
  mBurstFrames = mPlayingStream->getFramesPerBurst();
  mXRunLast = 0;
  mHeartbeatLast = 0.0f;
  if (mBurstFrames > 0)
    mPlayingStream->setBufferSizeInFrames(mBurstFrames * 2);

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
  return {mDeviceRate, 1};
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

  // Adaptive buffer sizing: on new XRuns (device-side underruns, i.e. we missed a
  // callback deadline), grow the buffer one burst at a time up to a cap. Both calls
  // are documented as safe from within the data callback.
  auto xrun = audioStream->getXRunCount();
  if (xrun && xrun.value() > mXRunLast)
  {
    mXRunLast = xrun.value();
    if (mBurstFrames > 0)
    {
      int32_t cap = mBurstFrames * 8;
      int32_t cur = audioStream->getBufferSizeInFrames();
      int32_t next = std::min(cur + mBurstFrames, cap);
      if (next > cur)
        audioStream->setBufferSizeInFrames(next);
    }
  }

  // Device-side heartbeat (Step 0): surfaces XRuns/buffer growth so device glitches
  // can be told apart from network/jitter impairments.
  float t = now_ms();
  if (mHeartbeatLast == 0.0f)
    mHeartbeatLast = t;
  else if (t - mHeartbeatLast >= 5000.0f)
  {
    ICELogInfo(<< "[spk-heartbeat] xruns=" << mXRunLast
               << " bufFrames=" << audioStream->getBufferSizeInFrames()
               << " burst=" << mBurstFrames
               << " rate=" << mDeviceRate);
    mHeartbeatLast = t;
  }

  return oboe::DataCallbackResult::Continue;
}

// Disconnect recovery: on a route change (headset/BT plug/unplug) AAudio tears the
// stream down and calls this on its own thread after the stream is closed. Rebuild
// on the new default route so audio doesn't silently die mid-call.
// See https://github.com/google/oboe/blob/master/docs/notes/disconnect.md
void AndroidOutputDevice::onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result) {
  if (result == oboe::Result::ErrorDisconnected && !mInShutdown) {
    ICELogInfo(<< "Output stream disconnected; restarting on the new route");
    // close() and open() each take mMutex internally; this callback holds none.
    close();
    open();
  }
}
#endif // TARGET_ANDROID
