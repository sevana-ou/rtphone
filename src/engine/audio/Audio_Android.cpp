#include "Audio_Android.h"
#include "../helper/HL_Sync.h"
#include "../helper/HL_Log.h"
#include <mutex>
#include <iostream>
#include "../helper/HL_String.h"

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

// -----------------------

OpenSLEngine::OpenSLEngine()
{}

OpenSLEngine::~OpenSLEngine()
{}

void OpenSLEngine::open()
{
  std::unique_lock<std::mutex> l(mMutex);
  if (++mUsageCounter == 1)
    internalOpen();

}

void OpenSLEngine::close()
{
  std::unique_lock<std::mutex> l(mMutex);
  if (mUsageCounter == 0)
    return;

  if (--mUsageCounter == 0)
    internalClose();
}

#define CHECK_OPENSLES_ERROR if (resultCode != SL_RESULT_SUCCESS) throw Exception(ERR_OPENSLES, (int)resultCode)

void OpenSLEngine::internalOpen()
{
  SLresult resultCode;

  // Instantiate OpenSL ES engine object
  resultCode = slCreateEngine(&mEngineObject, 0, nullptr, 0, nullptr, nullptr);
  CHECK_OPENSLES_ERROR;

  // Bring it online (realize)
  resultCode = (*mEngineObject)->Realize(mEngineObject, SL_BOOLEAN_FALSE);
  CHECK_OPENSLES_ERROR;

  // Get interface finally
  resultCode = (*mEngineObject)->GetInterface(mEngineObject, SL_IID_ENGINE, &mEngineInterface);
  CHECK_OPENSLES_ERROR;

  ICELogInfo(<< "OpenSL engine object created.");
}

void OpenSLEngine::internalClose()
{
  if (mEngineObject != nullptr)
  {
    ICELogInfo(<< "Destroy OpenSL engine object.");

    (*mEngineObject)->Destroy(mEngineObject);
    mEngineObject = nullptr;
    mEngineInterface = nullptr;
  }
}

SLEngineItf OpenSLEngine::getNativeEngine() const
{
  return mEngineInterface;
}

static OpenSLEngine OpenSLEngineInstance;

OpenSLEngine& OpenSLEngine::instance()
{
  return OpenSLEngineInstance;
}

// --------------- Input implementation ----------------
AndroidInputDevice::AndroidInputDevice(int devId)
{}

AndroidInputDevice::~AndroidInputDevice()
{}

static int RateToProbe[12][2] = {
        { SL_SAMPLINGRATE_16, 16000 },
        { SL_SAMPLINGRATE_8, 8000 },
        { SL_SAMPLINGRATE_32, 32000 },
        { SL_SAMPLINGRATE_44_1, 44100 },
        { SL_SAMPLINGRATE_11_025, 10025 },
        { SL_SAMPLINGRATE_22_05, 22050 },
        { SL_SAMPLINGRATE_24, 24000 },
        { SL_SAMPLINGRATE_48, 48000 },
        { SL_SAMPLINGRATE_64, 64000 },
        { SL_SAMPLINGRATE_88_2, 88200 },
        { SL_SAMPLINGRATE_96, 96000 },
        { SL_SAMPLINGRATE_192, 192000} };

bool AndroidInputDevice::open()
{
  if (active())
    return true;

  OpenSLEngine::instance().open();

  // Probe few sampling rates
  bool opened = false;
  for (int rateIndex = 0; rateIndex < 12 && !opened; rateIndex++)
  {
    try
    {
      internalOpen(RateToProbe[rateIndex][0], RateToProbe[rateIndex][1]);
      mDeviceRate = RateToProbe[rateIndex][1];
      ICELogInfo(<< "Input Opened with rate " << mDeviceRate << " and rate index " << rateIndex);
      opened = mDeviceRate != 0;
      if (!opened)
        internalClose();
    }
    catch(...)
    {
      opened = false;
      internalClose();
    }
  }
  mActive = opened;

  return opened;
}

void AndroidInputDevice::close()
{
  // There is no check for active() value because close() can be called to cleanup after bad open() call.
  internalClose();
  OpenSLEngine::instance().close();
  mActive = false;
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
  std::unique_lock<std::mutex> l(mMutex);
  while (mSdkRateCache.filled() < AUDIO_MIC_BUFFER_SIZE)
  {
    mDataCondVar.wait(l);
  }

  return mSdkRateCache.read(buffer, AUDIO_MIC_BUFFER_SIZE);
}

#define CHECK_SL_INTERFACE(INTF, ERR) {if (!INTF) throw Exception(ERR_OPENSLES, ERR); if (!(*INTF)) throw Exception(ERR_OPENSLES, ERR);}

void AndroidInputDevice::internalOpen(int rateCode, int rate)
{
  SLresult resultCode = 0;
  SLuint32  nrOfChannels = 1;

  // Prepare audio source
  SLDataLocator_IODevice devDescription = { SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT, SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
  SLDataSource audioSource = { &devDescription, NULL };

  // Source flags
  SLuint32 speakersFlags = nrOfChannels > 1 ? (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT) : SL_SPEAKER_FRONT_CENTER;

  // Buffer queue
  SLDataLocator_AndroidSimpleBufferQueue queueDescription = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };

  // Audio format
  SLDataFormat_PCM formatDescription = { SL_DATAFORMAT_PCM, nrOfChannels, (SLuint32)rateCode, SL_PCMSAMPLEFORMAT_FIXED_16,
                                         SL_PCMSAMPLEFORMAT_FIXED_16, (SLuint32)speakersFlags, SL_BYTEORDER_LITTLEENDIAN };

  SLDataSink audioSink = { &queueDescription, &formatDescription };

  // Create recorder
  // Do not forget about RECORD_AUDIO permission
  const SLInterfaceID interfacesList[2] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE, SL_IID_ANDROIDCONFIGURATION };
  const SLboolean interfacesRequirements[2] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

  // Get access to OpenSL engine
  SLEngineItf engine_interface = OpenSLEngine::instance().getNativeEngine();
  CHECK_SL_INTERFACE(engine_interface, -1);

  resultCode = (*engine_interface)->CreateAudioRecorder(
          OpenSLEngine::instance().getNativeEngine(),
          &mRecorderObject, &audioSource, &audioSink, 2, interfacesList, interfacesRequirements);
  CHECK_OPENSLES_ERROR;
  CHECK_SL_INTERFACE(mRecorderObject, -2);

  // Obtain stream type
  resultCode = (*mRecorderObject)->GetInterface(mRecorderObject, SL_IID_ANDROIDCONFIGURATION, &mAndroidCfg);
  CHECK_OPENSLES_ERROR;

  // Now audio recorder goes to real world
  resultCode = (*mRecorderObject)->Realize(mRecorderObject, SL_BOOLEAN_FALSE);
  CHECK_OPENSLES_ERROR;

  // Get recorder interface
  resultCode = (*mRecorderObject)->GetInterface(mRecorderObject, SL_IID_RECORD, &mRecorderInterface);
  CHECK_OPENSLES_ERROR;
  CHECK_SL_INTERFACE(mRecorderInterface, -3);

  // Now buffer queue interface...
  resultCode = (*mRecorderObject)->GetInterface(mRecorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &mRecorderBufferInterface);
  CHECK_OPENSLES_ERROR;
  CHECK_SL_INTERFACE(mRecorderBufferInterface, -4);

  // Resampler is needed to provide SDK's rate
  mResampler = std::make_shared<Resampler>();
  mResampler->start(nrOfChannels, rate, AUDIO_SAMPLERATE);

  // Allocate recorder buffer size
  mBufferSize = (AUDIO_MIC_BUFFER_LENGTH / 10) * (rate / 100) * 2;
  mRecorderBuffer.setCapacity(mBufferSize * AUDIO_MIC_BUFFER_COUNT);
  mRecorderBufferIndex = 0;

  // Setup data consuming callback
  resultCode = (*mRecorderBufferInterface)->RegisterCallback(mRecorderBufferInterface, DeviceCallback, (void*)this);
  CHECK_OPENSLES_ERROR;

  // Setup buffers
  for (int i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
    (*mRecorderBufferInterface)->Enqueue(mRecorderBufferInterface, mRecorderBuffer.data() + i * mBufferSize, mBufferSize);

  // Start finally
  resultCode = (*mRecorderInterface)->SetRecordState(mRecorderInterface, SL_RECORDSTATE_RECORDING);
  CHECK_OPENSLES_ERROR;
}

void AndroidInputDevice::internalClose()
{
  if (!mRecorderObject)
    return;

  if (*mRecorderObject)
  {
    if (active())
    {
      // Stop recording
      (*mRecorderInterface)->SetRecordState(mRecorderInterface, SL_RECORDSTATE_STOPPED);

      // Wait until recording will not stop really
      SLuint32  state = SL_RECORDSTATE_STOPPED;
      do
      {
        (*mRecorderInterface)->GetRecordState(mRecorderInterface, &state);
        SyncHelper::delay(1);
      }
      while (state == SL_RECORDSTATE_RECORDING);
    }
    (*mRecorderObject)->Destroy(mRecorderObject);
  }

  mRecorderObject = nullptr;
  mRecorderInterface = nullptr;
  mRecorderBufferInterface = nullptr;
  mAndroidCfg = nullptr;
}

void AndroidInputDevice::handleCallback(SLAndroidSimpleBufferQueueItf bq)
{
  std::unique_lock<std::mutex> l(mMutex);

  // Send data to AudioPair
  if (mConnection)
    mConnection->onMicData(getFormat(), mRecorderBuffer.data() + mRecorderBufferIndex * mBufferSize, mBufferSize);
  /*
  // Send audio to cache with native sample rate
  mDeviceRateCache.add(mRecorderBuffer.data() + mRecorderBufferIndex * mBufferSize, mBufferSize);

  // Check if there is enough data (10 ms) to send
  int tenMsSize = (int)Format(mDeviceRate, 1).sizeFromTime(10);
  while (mDeviceRateCache.filled() >= tenMsSize)
  {
    char* resampled = (char*)alloca(Format().sizeFromTime(10));
    int processed = 0;
    int outlen = mResampler->processBuffer(mDeviceRateCache.data(), tenMsSize, processed, resampled, Format().sizeFromTime(10));
    if (outlen > 0)
      mSdkRateCache.add(resampled, (int)Format().sizeFromTime(10));
    mDeviceRateCache.erase(tenMsSize);
  }

  // Tell about data
  while (mSdkRateCache.filled() >= AUDIO_MIC_BUFFER_SIZE)
  {
    if (mConnection)
      mConnection->onMicData(Format(), mSdkRateCache.data(), AUDIO_MIC_BUFFER_SIZE);
    mSdkRateCache.erase(AUDIO_MIC_BUFFER_SIZE);
  }
  */
  // Re-enqueue used buffer
  (*mRecorderBufferInterface)->Enqueue(mRecorderBufferInterface, mRecorderBuffer.data() + mRecorderBufferIndex * mBufferSize, mBufferSize);
  mRecorderBufferIndex++;
  mRecorderBufferIndex %= AUDIO_MIC_BUFFER_COUNT;
}

void AndroidInputDevice::DeviceCallback(SLAndroidSimpleBufferQueueItf bq, void *context)
{
  try
  {
    if (context)
      reinterpret_cast<AndroidInputDevice*>(context)->handleCallback(bq);
  }
  catch(...)
  {}
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
  bool opened = false;
  for (int rateIndex = 0; rateIndex < 12 && !opened; rateIndex++)
  {
    try
    {
      internalOpen(RateToProbe[rateIndex][0], RateToProbe[rateIndex][1], true);
      opened = true;
      mDeviceRate = RateToProbe[rateIndex][1];
      ICELogCritical(<< "Output opened with rate " << mDeviceRate << " and index " << rateIndex);
    }
    catch(...)
    {
      opened = false;
    }
  }
  if (opened)
    ICELogInfo(<< "Speaker opened on rate " << mDeviceRate);

  return opened;
}

void AndroidOutputDevice::close()
{
  std::unique_lock<std::mutex> l(mMutex);
  internalClose();
}

Format AndroidOutputDevice::getFormat()
{
  return Format(mDeviceRate, 1);
}

bool AndroidOutputDevice::fakeMode()
{
  return false;
}

void AndroidOutputDevice::setFakeMode(bool fakemode)
{
}

void AndroidOutputDevice::internalOpen(int rateId, int rate, bool voice)
{
  mInShutdown = false;

  SLresult resultCode;
  SLuint32 channels = 1;

  // Configure audio source
  SLDataLocator_AndroidSimpleBufferQueue queue_desc = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2 };

  const SLInterfaceID interfacesList[] = { SL_IID_VOLUME };
  const SLboolean interfaceRequirements[] = { SL_BOOLEAN_FALSE };
  resultCode = (*OpenSLEngine::instance().getNativeEngine())->CreateOutputMix(
          OpenSLEngine::instance().getNativeEngine(), &mMixer, 1, interfacesList,
          interfaceRequirements);
  CHECK_OPENSLES_ERROR;

  // Bring mixer online
  resultCode = (*mMixer)->Realize(mMixer, SL_BOOLEAN_FALSE);
  CHECK_OPENSLES_ERROR;

  // Prepare mixer configuration
  SLuint32 speakers =
          channels > 1 ? (SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT) : SL_SPEAKER_FRONT_CENTER;

  // Describe audio format
  SLDataFormat_PCM pcm_format = {SL_DATAFORMAT_PCM, channels, (SLuint32) rateId,
                                  SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                                  speakers, SL_BYTEORDER_LITTLEENDIAN};

  // Describe audio source - buffers + audio format
  SLDataSource audio_source = { &queue_desc, &pcm_format };

  // Describe audio sink
  SLDataLocator_OutputMix mixer_desc = { SL_DATALOCATOR_OUTPUTMIX, mMixer };
  SLDataSink audio_sink = { &mixer_desc, NULL };

  // Create player instance
  const SLInterfaceID playerInterfaces[] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                             SL_IID_VOLUME,
                                             SL_IID_ANDROIDCONFIGURATION };
  const SLboolean playerInterfacesReqs[] = { SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE };

  resultCode = (*OpenSLEngine::instance().getNativeEngine())->CreateAudioPlayer(
          OpenSLEngine::instance().getNativeEngine(), &mPlayer,
          &audio_source, &audio_sink, 3, playerInterfaces, playerInterfacesReqs);
  CHECK_OPENSLES_ERROR;

  // Get android config interface
  resultCode = (*mPlayer)->GetInterface(mPlayer, SL_IID_ANDROIDCONFIGURATION, &mAndroidConfig);

  if (resultCode == SL_RESULT_SUCCESS)
  {
    SLint32 streamType = voice ? SL_ANDROID_STREAM_VOICE : SL_ANDROID_STREAM_MEDIA;
    resultCode = (*mAndroidConfig)->SetConfiguration(mAndroidConfig, SL_ANDROID_KEY_STREAM_TYPE,
                                                     &streamType, sizeof(SLint32));
    if (resultCode != SL_RESULT_SUCCESS)
      ICELogCritical(<< "Failed to set audio destination with error " << (unsigned)resultCode);
  }
  else
    ICELogCritical(<< "Failed to obtain android cfg audio interface with error " << (unsigned)resultCode);

  // Bring player online
  resultCode = (*mPlayer)->Realize(mPlayer, SL_BOOLEAN_FALSE);
  CHECK_OPENSLES_ERROR;

  // Obtain player control
  resultCode = (*mPlayer)->GetInterface(mPlayer, SL_IID_PLAY, &mPlayerControl);
  CHECK_OPENSLES_ERROR;

  // Get the buffer queue interface
  resultCode = (*mPlayer)->GetInterface(mPlayer, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                        &mBufferQueue);
  CHECK_OPENSLES_ERROR;

  // Setup callback
  resultCode = (*mBufferQueue)->RegisterCallback(mBufferQueue, DeviceCallback, this);
  CHECK_OPENSLES_ERROR;

  // Enqueue buffers
  mBufferSize = (int)Format(rate, channels).sizeFromTime(AUDIO_SPK_BUFFER_LENGTH);
  mPlayBuffer.setCapacity(AUDIO_SPK_BUFFER_COUNT * mBufferSize);

  mBufferIndex = 0;
  for (int i = 0; i < AUDIO_SPK_BUFFER_COUNT; i++)
    (*mBufferQueue)->Enqueue(mBufferQueue, mPlayBuffer.data() + i * mBufferSize,
                             (SLuint32)mBufferSize);

  // Set the player's state to playing
  resultCode = (*mPlayerControl)->SetPlayState(mPlayerControl, SL_PLAYSTATE_PLAYING);
  CHECK_OPENSLES_ERROR;

  ICELogInfo(<< "Android audio output is opened and playing.");
}

void AndroidOutputDevice::internalClose()
{
  if (mPlayer)
  {
    if (*mPlayer)
    {
      mInShutdown = true;
      ICELogInfo(<< "Stop player");
      if (mPlayerControl) {
        if (*mPlayerControl) {
          SLuint32 state = SL_PLAYSTATE_PLAYING;
          (*mPlayerControl)->SetPlayState(mPlayerControl, SL_PLAYSTATE_STOPPED);

          while (state != SL_PLAYSTATE_STOPPED) {
            (*mPlayerControl)->GetPlayState(mPlayerControl, &state);
            SyncHelper::delay(1);
          }
        }
      }

      // Clear buffer queue
      ICELogInfo(<< "Clear player buffer queue");
      (*mBufferQueue)->Clear(mBufferQueue);

      ICELogInfo(<< "Destroy player object");
      // Destroy player object
      (*mPlayer)->Destroy(mPlayer);

      ICELogInfo(<< "Android audio output closed.");

      mPlayer = nullptr;
      mPlayerControl = nullptr;
      mBufferQueue = nullptr;
      mEffect = nullptr;
      mAndroidConfig = nullptr;
    }
  }

  if (mMixer)
  {
    if (*mMixer)
      (*mMixer)->Destroy(mMixer);
    mMixer = nullptr;
  }
}

void AndroidOutputDevice::handleCallback(SLAndroidSimpleBufferQueueItf bq)
{
  if (mInShutdown)
    return;
  /*{
    char silence[mBufferSize]; memset(silence, 0, mBufferSize);
    (*mBufferQueue)->Enqueue(mBufferQueue, silence, mBufferSize);
    return;
  }*/

  // Ask producer about data
  char* buffer = mPlayBuffer.mutableData() + mBufferIndex * mBufferSize;
  if (mConnection)
  {
    Format f = getFormat();
    if (f.mRate != 0)
      mConnection->onSpkData(f, buffer, mBufferSize);
  }
  (*mBufferQueue)->Enqueue(mBufferQueue, buffer, (SLuint32)mBufferSize);

  mBufferIndex++;
  mBufferIndex %= AUDIO_SPK_BUFFER_COUNT;
}

void AndroidOutputDevice::DeviceCallback(SLAndroidSimpleBufferQueueItf bq, void* context)
{
  if (!context)
    return;

  try
  {
    reinterpret_cast<AndroidOutputDevice*>(context)->handleCallback(bq);
  }
  catch(...)
  {}
}

#endif // TARGET_ANDROID