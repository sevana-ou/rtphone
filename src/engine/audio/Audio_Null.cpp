#include "Audio_Null.h"
#include "helper/HL_Log.h"
#include <assert.h>
#include <chrono>
#define LOG_SUBSYSTEM "NULL audio"

using namespace Audio;

NullTimer::NullTimer(int interval, Delegate *delegate, const char* name)
  :mShutdown(false), mDelegate(delegate), mInterval(interval), mThreadName(name)
{
  start();
}

NullTimer::~NullTimer()
{
  stop();
}

void NullTimer::start()
{
  mShutdown = false;
  mWorkerThread = std::thread(&NullTimer::run, this);
}

void NullTimer::stop()
{
  mShutdown = true;
  if (mWorkerThread.joinable())
    mWorkerThread.join();
}

void NullTimer::run()
{
  mTail = 0;
  while (!mShutdown)
  {
    // Get current timestamp
    std::chrono::system_clock::time_point timestamp = std::chrono::system_clock::now();

    while (mTail >= mInterval * 1000)
    {
      if (mDelegate)
        mDelegate->onTimerSignal(*this);
      mTail -= mInterval * 1000;
    }

    // Sleep for mInterval - mTail milliseconds
    std::this_thread::sleep_for(std::chrono::microseconds(mInterval * 1000 - mTail));

    mTail += (int)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - timestamp).count();
  }
}

// --------------------- NullInputDevice -------------------------
NullInputDevice::NullInputDevice()
  :mBuffer(nullptr)
{
}

NullInputDevice::~NullInputDevice()
{
  close();
}

bool NullInputDevice::open()
{
  mBuffer = malloc(AUDIO_MIC_BUFFER_SIZE);
  memset(mBuffer, 0, AUDIO_MIC_BUFFER_SIZE);
  mTimeCounter = 0; mDataCounter = 0;
  // Creation of timer starts it also. So first onTimerSignal can come even before open() returns.
  mTimer = std::make_shared<NullTimer>(AUDIO_MIC_BUFFER_LENGTH, this, "NullMicrophoneThread");
  return true;
}

void NullInputDevice::close()
{
  mTimer.reset();
  if (mBuffer)
  {
    free(mBuffer);
    mBuffer = nullptr;
  }
  ICELogInfo(<<"Pseudocaptured " << mTimeCounter << " milliseconds , " << mDataCounter << " bytes.");
}

Format NullInputDevice::getFormat()
{
  assert (Format().sizeFromTime(AUDIO_MIC_BUFFER_LENGTH) == AUDIO_MIC_BUFFER_SIZE);
  return Format();
}

void NullInputDevice::onTimerSignal(NullTimer& timer)
{
  mTimeCounter += AUDIO_MIC_BUFFER_LENGTH;
  mDataCounter += AUDIO_MIC_BUFFER_SIZE;
  if (mConnection)
    mConnection->onMicData(getFormat(), mBuffer, AUDIO_MIC_BUFFER_SIZE);
}

// --------------------- NullOutputDevice --------------------------
NullOutputDevice::NullOutputDevice()
  :mBuffer(nullptr)
{
}

NullOutputDevice::~NullOutputDevice()
{
  close();
}


bool NullOutputDevice::open()
{
  mTimeCounter = 0; mDataCounter = 0;
  mBuffer = malloc(AUDIO_SPK_BUFFER_SIZE);
  // Creation of timer starts it also. So first onSpkData() can come before open() returns even.
  mTimer = std::make_shared<NullTimer>(AUDIO_SPK_BUFFER_LENGTH, this, "NullSpeakerThread");
  return true;
}

void NullOutputDevice::close()
{
  mTimer.reset();
  free(mBuffer); mBuffer = nullptr;
  ICELogInfo(<< "Pseudoplayed " << mTimeCounter << " milliseconds, " << mDataCounter << " bytes.");
}

Format NullOutputDevice::getFormat()
{
  assert (Format().sizeFromTime(AUDIO_SPK_BUFFER_LENGTH) == AUDIO_SPK_BUFFER_SIZE);
  return Format();
}

void NullOutputDevice::onTimerSignal(NullTimer &timer)
{
  mTimeCounter += AUDIO_SPK_BUFFER_LENGTH;
  mDataCounter += AUDIO_SPK_BUFFER_SIZE;
  if (mConnection)
    mConnection->onSpkData(getFormat(), mBuffer, AUDIO_SPK_BUFFER_SIZE);
}

// ---------------------- NullEnumerator --------------------------
NullEnumerator::NullEnumerator()
{}

NullEnumerator::~NullEnumerator()
{}

void NullEnumerator::open(int direction)
{}

void NullEnumerator::close()
{}

int NullEnumerator::count()
{
  return 1;
}

std::tstring NullEnumerator::nameAt(int index)
{
#if defined(TARGET_WIN)
  return L"null";
#else
  return "null";
#endif
}

int NullEnumerator::idAt(int index)
{
  return 0;
}

int NullEnumerator::indexOfDefaultDevice()
{
  return 0;
}

