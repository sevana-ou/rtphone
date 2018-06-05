/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#if defined(TARGET_WIN) && defined(_MSC_VER)

#include "Audio_DirectSound.h"
#include "Audio_Helper.h"
#include "../Helper/HL_Exception.h"
#include "../Helper/HL_Log.h"

#include <dsconf.h>
#include <process.h>
#pragma comment(lib, "dsound.lib")
#pragma comment(lib, "dxguid.lib")

#define DRVM_MAPPER_CONSOLEVOICECOM_GET (0x2000 + 23)
#define DRVM_MAPPER_PREFERRED_GET (0x2000 + 21)

#define DRV_QUERYFUNCTIONINSTANCEID  (DRV_RESERVED + 17)
#define DRV_QUERYFUNCTIONINSTANCEIDSIZE (DRV_RESERVED + 18)

#define LOG_SUBSYSTEM "DirectSound"

using namespace Audio;

class DSoundInit
{
public:
  DSoundInit();
  virtual ~DSoundInit();

  void load();
  void unload();

  struct EntryPoints
  {
    HINSTANCE mInstance;

    HRESULT (WINAPI *DirectSoundCreate8)(LPGUID, LPDIRECTSOUND8 *, LPUNKNOWN);
    HRESULT (WINAPI *DirectSoundEnumerateW)(LPDSENUMCALLBACKW, LPVOID);
    HRESULT (WINAPI *DirectSoundEnumerateA)(LPDSENUMCALLBACKA, LPVOID);

    HRESULT (WINAPI *DirectSoundCaptureCreate8)(LPGUID, LPDIRECTSOUNDCAPTURE8* , LPUNKNOWN);
    HRESULT (WINAPI *DirectSoundCaptureEnumerateW)(LPDSENUMCALLBACKW, LPVOID);
    HRESULT (WINAPI *DirectSoundCaptureEnumerateA)(LPDSENUMCALLBACKA, LPVOID);
    HRESULT (WINAPI *GetDeviceID)(LPCGUID src, LPGUID dst);
  } mRoutines;

protected:
  LPDIRECTSOUND mDirectSound;
  Mutex mGuard;
  unsigned int  mRefCount;
};

DSoundInit gDSoundInit;

DSoundInit::DSoundInit()
  :mRefCount(0)
{
}

DSoundInit::~DSoundInit()
{
  //Unload();
}

void DSoundInit::load()
{
  Lock l(mGuard);

  if (++mRefCount == 1)
  {
    HRESULT hr = E_FAIL;

    hr = ::CoInitialize(NULL);

    //load the DirectSound DLL
    mRoutines.mInstance = ::LoadLibrary(L"dsound.dll");
    if (!mRoutines.mInstance)
      throw std::logic_error("Cannot load dsound.dll");

    mRoutines.DirectSoundCaptureCreate8 = (HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUNDCAPTURE8 *, LPUNKNOWN))::GetProcAddress(mRoutines.mInstance, "DirectSoundCaptureCreate8");
    mRoutines.DirectSoundCaptureEnumerateW = (HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))::GetProcAddress(mRoutines.mInstance, "DirectSoundCaptureEnumerateW");
    mRoutines.DirectSoundCreate8 = (HRESULT (WINAPI *)(LPGUID, LPDIRECTSOUND8 *, LPUNKNOWN))::GetProcAddress(mRoutines.mInstance, "DirectSoundCreate8");
    mRoutines.DirectSoundEnumerateW = (HRESULT (WINAPI *)(LPDSENUMCALLBACKW, LPVOID))::GetProcAddress(mRoutines.mInstance, "DirectSoundEnumerateW");
    mRoutines.GetDeviceID = (HRESULT (WINAPI*) (LPCGUID, LPGUID)) GetProcAddress(mRoutines.mInstance, "GetDeviceID");
  }
}

void DSoundInit::unload()
{
  Lock l(mGuard);
  if (--mRefCount == 0)
  {
    if (mRoutines.mInstance)
    {
      ::FreeLibrary(mRoutines.mInstance);
      mRoutines.mInstance = NULL;
    }

    CoUninitialize();
  }
}

// --------------- VistaEnumerator ---------------------
VistaEnumerator::VistaEnumerator()
:mCollection(NULL), mDefaultDevice(NULL), mEnumerator(NULL), mDirection(eCapture)
{
}

VistaEnumerator::~VistaEnumerator()
{
  close();
}

void VistaEnumerator::open(int direction)
{
  const CLSID CLSID_MMDeviceEnumerator = __uuidof(MMDeviceEnumerator);
  const IID IID_IMMDeviceEnumerator = __uuidof(IMMDeviceEnumerator);

  mDirection = (direction == myMicrophone) ? eCapture : eRender;

  HRESULT hr = CoCreateInstance(
    CLSID_MMDeviceEnumerator, NULL,
    CLSCTX_ALL, IID_IMMDeviceEnumerator,
    (void**)&mEnumerator);
  if (!mEnumerator)
    return;

  hr = mEnumerator->EnumAudioEndpoints(mDirection, DEVICE_STATE_ACTIVE, &mCollection);
  if (!mCollection)
    return;
  hr = mEnumerator->GetDefaultAudioEndpoint(mDirection, eCommunications, &mDefaultDevice);
  if (!mDefaultDevice)
    return;

  enumerate();
}

void VistaEnumerator::close()
{
  try
  {
    if (mCollection)
    {
      mCollection->Release(); 
      mCollection = NULL;
    }

    if (mDefaultDevice)
    {
      //mDefaultDevice->Release(); 
      mDefaultDevice = NULL;
    }

    if (mEnumerator)
    {
      mEnumerator->Release(); 
      mEnumerator = NULL;
    }
  }
  catch(...)
  {
  }
}

IMMDevice* VistaEnumerator::mapIndexToInterface(int index)
{
  if (!mCollection)
    return NULL;

  if (index == -1)
    return mDefaultDevice;

  size_t idSize = 0;
  MMRESULT mmres  = 0;
  WCHAR* id = NULL;
  if (mDirection == eCapture)
  {
    mmres = waveInMessage((HWAVEIN)index, DRV_QUERYFUNCTIONINSTANCEIDSIZE, (DWORD_PTR)&idSize, NULL);

    if (mmres != MMSYSERR_NOERROR)
      return NULL;

    id = (WCHAR*)_alloca(idSize*sizeof(WCHAR));
    mmres = waveInMessage((HWAVEIN)index, DRV_QUERYFUNCTIONINSTANCEID, (DWORD_PTR)id, idSize);
  }
  else
  {
    mmres = waveOutMessage((HWAVEOUT)index, DRV_QUERYFUNCTIONINSTANCEIDSIZE, (DWORD_PTR)&idSize, NULL);

    if (mmres != MMSYSERR_NOERROR)
      return NULL;

    id = (WCHAR*)_alloca(idSize*sizeof(WCHAR));
    mmres = waveOutMessage((HWAVEOUT)index, DRV_QUERYFUNCTIONINSTANCEID, (DWORD_PTR)id, idSize);
  }

  if (mmres != MMSYSERR_NOERROR)
    return NULL;

  IMMDevice* pDevice = NULL;
  mEnumerator->GetDevice(id, &pDevice);

  return pDevice;    
}

void VistaEnumerator::enumerate()
{
  mNameList.clear();
  int res = (int)count();

  for (int i=0; i<res; i++)
  {
    IMMDevice* dev = mapIndexToInterface(i);
    if (dev)
    {
      IPropertyStore* store = NULL;
      dev->OpenPropertyStore(STGM_READ, &store);
      if (store)
      {
        PROPVARIANT varName;
        PropVariantInit(&varName);
        if (store->GetValue(PKEY_Device_FriendlyName, &varName) == S_OK)
          mNameList.push_back(varName.pwszVal);
        PropVariantClear(&varName);
        store->Release();
      }
      dev->Release();
    }
  }
}

std::tstring VistaEnumerator::nameAt(int index)
{
  return mNameList[index];  
}

int VistaEnumerator::idAt(int index)
{
  return index;
}

int VistaEnumerator::count()
{
  if (mDirection == eCapture)
    return waveInGetNumDevs();
  else
    return waveOutGetNumDevs();
}

int VistaEnumerator::indexOfDefaultDevice()
{
  DWORD devID = -1, status = 0;

  if (mDirection == mySpeaker)
  {
    if (waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status) != MMSYSERR_NOERROR)
      waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status);
  }
  else
  {
    if (waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status) != MMSYSERR_NOERROR)
      waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status);
  }
  return devID;
}

// -------------- XpEnumerator ---------------
XpEnumerator::XpEnumerator()
:mDirection(-1)
{
}

XpEnumerator::~XpEnumerator()
{
}

void XpEnumerator::open(int direction)
{
  mNameList.clear();
  if (direction == myMicrophone)
  {
    int count = waveInGetNumDevs();
    for (int i=0; i<count; i++)
    {
      WAVEINCAPS caps;
      if (waveInGetDevCaps(i, &caps, sizeof caps) == MMSYSERR_NOERROR)
        mNameList.push_back(caps.szPname);
      else
        mNameList.push_back(L"Bad device");
    }
  }
  else
  {
    int count = waveOutGetNumDevs();
    for (int i=0; i<count; i++)
    {
      WAVEOUTCAPS caps;
      if (waveOutGetDevCaps(i, &caps, sizeof caps) == MMSYSERR_NOERROR)
        mNameList.push_back(caps.szPname);
      else
        mNameList.push_back(L"Bad device");
    }
  }
}

void XpEnumerator::close()
{
}

int XpEnumerator::count()
{
  return mNameList.size();
}

std::tstring XpEnumerator::nameAt(int index)
{
  return mNameList[index];
}

int XpEnumerator::idAt(int index)
{
  return index;
}

int XpEnumerator::indexOfDefaultDevice()
{
  DWORD devID = -1, status = 0;

  if (mDirection == mySpeaker)
  {
    if (waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status) != MMSYSERR_NOERROR)
      waveOutMessage((HWAVEOUT)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status);
  }
  else
  {
    if (waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_CONSOLEVOICECOM_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status) != MMSYSERR_NOERROR)
      waveInMessage((HWAVEIN)WAVE_MAPPER, DRVM_MAPPER_PREFERRED_GET, (DWORD_PTR)&devID, (DWORD_PTR)&status);
  }
  return devID;
}

// -------- DSoundInputDevice ---------------
DSoundInputDevice::DSoundInputDevice(GUID deviceId)
:mSimulate(false), mBufferIndex(0), mGUID(deviceId), mThreadHandle(0), mDenoiser(AUDIO_SAMPLERATE), mEnableDenoiser(true),
mNullAudio(AUDIO_MIC_BUFFER_LENGTH, AUDIO_MIC_BUFFER_COUNT)
#ifdef AUDIO_DUMPINPUT
,mDump(AUDIO_SAMPLERATE)
#endif
{
  gDSoundInit.load();

  mSimulate = false;
  mNotifications = NULL;
  mDevice = NULL;
  mBuffer = NULL;
  mShutdownSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  for (unsigned i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
  {
    mEventArray[i].dwOffset = (i + 1) * AUDIO_MIC_BUFFER_SIZE - 1;
    mEventSignals[i] = mEventArray[i].hEventNotify = ::CreateEvent(NULL, TRUE, FALSE, NULL);
  }
  mRefCount = 0;
}

DSoundInputDevice::~DSoundInputDevice()
{
  close();
  ::CloseHandle(mShutdownSignal);
  for (int i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
    ::CloseHandle(mEventArray[i].hEventNotify);

  gDSoundInit.unload();
}

void DSoundInputDevice::enableDenoiser(bool enable)
{
  mEnableDenoiser = enable;
}

bool DSoundInputDevice::isSimulate() const
{
  return mSimulate;
}

void DSoundInputDevice::openDevice()
{ 
  ICELogInfo(<< "Open DirectSound audio input.")
  ::CoInitialize(NULL);
  Lock l(mGuard);
  // Ensure if GUID is not null
  if (IsEqualGUID(mGUID, GUID_NULL))
  {
    setSimulate( true );
    return;
  }
  
  
#ifdef AUDIO_DUMPINPUT
  mDump.open(L"audioinput.wav");
#endif
  
  mNextBuffer = 0; mDevice = NULL; IUnknown* unk = NULL; mBuffer = NULL;
  DSoundHelper::checkComResult(gDSoundInit.mRoutines.DirectSoundCaptureCreate8(&mGUID, &mDevice, NULL));

  WAVEFORMATEX wfx;
  memset(&wfx, 0, sizeof(wfx));

  //wfx.cbSize = sizeof(wfx);
  wfx.nChannels = AUDIO_CHANNELS;
  wfx.nSamplesPerSec = AUDIO_SAMPLERATE;
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;;
  wfx.nAvgBytesPerSec = AUDIO_SAMPLERATE * 2 * AUDIO_CHANNELS;
  wfx.wFormatTag = WAVE_FORMAT_PCM;

  DSCBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(dsbd));
  dsbd.dwSize = sizeof(DSCBUFFERDESC);
  dsbd.dwFlags = 0;//DSBCAPS_CTRLPOSITIONNOTIFY;
  dsbd.dwBufferBytes = AUDIO_MIC_BUFFER_COUNT * AUDIO_MIC_BUFFER_SIZE;
  dsbd.lpwfxFormat = &wfx;
  dsbd.dwFXCount = 0;
  dsbd.lpDSCFXDesc = NULL;
  
  IDirectSoundCaptureBuffer* dscb = NULL;
  DSoundHelper::checkComResult(mDevice->CreateCaptureBuffer(&dsbd, &dscb, NULL));
  DSoundHelper::checkComResult(dscb->QueryInterface(IID_IDirectSoundCaptureBuffer8, (void**)&mBuffer));
  DSoundHelper::checkComResult(dscb->QueryInterface(IID_IDirectSoundNotify, (void**)&mNotifications));
  DSoundHelper::checkComResult(mNotifications->SetNotificationPositions(AUDIO_MIC_BUFFER_COUNT, mEventArray));
  DSoundHelper::checkComResult(mBuffer->Start(DSCBSTART_LOOPING));
  dscb->Release();
  setSimulate( false );
}

bool DSoundInputDevice::open()
{
  ICELogInfo(<< "Request to DirectSound audio input");
  Lock lock(mGuard);
  mRefCount++;
  if (mRefCount == 1)
  {
    ICELogInfo(<< "Schedule DirectSound audio input thread");
    mThreadHandle = (HANDLE)_beginthread(&threadProc, 0, this);
  }
  return true;
}

void DSoundInputDevice::closeDevice()
{
  ICELogInfo(<<"Close DirectSound audio input");
  Lock l(mGuard);

#ifdef AUDIO_DUMPINPUT
  mDump.close();
#endif

  if (mBuffer)    
  { 
    mBuffer->Stop(); 
    mBuffer->Release(); 
    mBuffer = NULL; 
  }
  if (mNotifications) 
  { 
    mNotifications->Release(); 
    mNotifications = NULL; 
  }
  if (mDevice)    
  { 
    mDevice->Release(); 
    mDevice = NULL; 
  } 
  else 
    return;

  ::CoUninitialize();
}

void DSoundInputDevice::close()
{
  {
    Lock l(mGuard);
    mRefCount--;
    if (mRefCount != 0)
      return;
  
    // Set shutdown signal
    if (!mThreadHandle)
      return;

    ::SetEvent(mShutdownSignal);
  }

  ::WaitForSingleObject(mThreadHandle, INFINITE);
  mThreadHandle = 0;
}

bool DSoundInputDevice::tryReadBuffer(void* buffer)
{
  // Ensure device exists
  if (!mDevice)
  {
    setSimulate( true );
    return false;
  }

  if (mQueue.size() >= AUDIO_MIC_BUFFER_SIZE)
  {
    memcpy(buffer, mQueue.data(), AUDIO_MIC_BUFFER_SIZE);
    if (mEnableDenoiser && AUDIO_CHANNELS == 1)
      mDenoiser.fromMic(buffer, AUDIO_MIC_BUFFER_LENGTH);

#ifdef AUDIO_DUMPINPUT
    mDump.write(buffer, AUDIO_MIC_BUFFER_SIZE);
#endif
    mQueue.erase(0, AUDIO_MIC_BUFFER_SIZE);
    return true;
  }

  try
  {
    if (::WaitForSingleObject(mEventArray[mNextBuffer].hEventNotify, AUDIO_MIC_BUFFER_COUNT * AUDIO_MIC_BUFFER_LENGTH * 4) != WAIT_OBJECT_0)
    {
      setSimulate( true );
      return false;
    }
  
    // See if all other buffers are signaled
    if (::WaitForMultipleObjects(AUDIO_MIC_BUFFER_COUNT, mEventSignals, TRUE, 0) != WAIT_TIMEOUT)
    {
      // Possible overflow. Consider current buffer resulting. Reset ALL events.
      for (int i = 0; i<AUDIO_MIC_BUFFER_COUNT; i++)
        ResetEvent(mEventArray[i].hEventNotify);
    }
    else
      ResetEvent(mEventArray[mNextBuffer].hEventNotify);

    // Find the buffer start offset
    mReadOffset = mNextBuffer * AUDIO_MIC_BUFFER_SIZE;
  
    //increase the buffer's index
    if (++mNextBuffer == AUDIO_MIC_BUFFER_COUNT)
      mNextBuffer = 0;
  
    LPVOID ptr1 = NULL, ptr2 = NULL; DWORD len1 = 0, len2 = 0;
    DSoundHelper::checkComResult(mBuffer->Lock(mReadOffset, AUDIO_MIC_BUFFER_SIZE, &ptr1, &len1, &ptr2, &len2, 0));
  
    // Copy&Enqueue captured data to mQueue
    if (ptr1 && len1)
      mQueue.appendBuffer(ptr1, len1);

    if (ptr2 && len2)
      mQueue.appendBuffer(ptr2, len2);
    
    DSoundHelper::checkComResult(mBuffer->Unlock(ptr1, len1, ptr2, len2));
    if (mQueue.size() >= AUDIO_MIC_BUFFER_SIZE)
    {
      memcpy(buffer, mQueue.data(), AUDIO_MIC_BUFFER_SIZE);
      if (mEnableDenoiser && AUDIO_CHANNELS == 1)
        mDenoiser.fromMic(buffer, AUDIO_MIC_BUFFER_LENGTH);

#ifdef AUDIO_DUMPINPUT
      mDump.write(buffer, AUDIO_MIC_BUFFER_SIZE);
#endif
      mQueue.erase(0, AUDIO_MIC_BUFFER_SIZE);
    }
    else
      return false;
  
    return true;
  }
  catch(...)
  {
    setSimulate( true );
  }
  return false;  
}

void DSoundInputDevice::setSimulate(bool s)
{
  if (!mSimulate && s)
    mNullAudio.start();
  else
  if (mSimulate && !s)
    mNullAudio.stop();

  mSimulate = s;
}

Format DSoundInputDevice::getFormat()
{
  return Format();
}

int DSoundInputDevice::readBuffer(void* buffer)
{
  //Lock lock(mGuard);
  if (mRefCount <= 0 || isSimulate())
    return 0;
  
  // Check for finished buffer
  if (!tryReadBuffer(buffer))
    return 0;
  
  return AUDIO_MIC_BUFFER_SIZE;
}

void DSoundInputDevice::threadProc(void* arg)
{
  DSoundInputDevice* impl = (DSoundInputDevice*)arg;
  
  impl->openDevice();
  
  while (true)
  {
    // Poll for shutdown signal
    if (::WaitForSingleObject(impl->mShutdownSignal, 0) == WAIT_OBJECT_0)
      break;
    
    // Preset buffer with silence
    memset(impl->mTempBuffer, 0, AUDIO_MIC_BUFFER_SIZE);
    
    // Try to read buffer
    if (!impl->readBuffer(impl->mTempBuffer))
    {
      // Introduce delay here to simulate true audio
      impl->mNullAudio.waitForBuffer();
    }

    // Distribute the captured buffer
    if (impl->connection())
      impl->connection()->onMicData(impl->getFormat(), impl->mTempBuffer, AUDIO_MIC_BUFFER_SIZE);
  }

  impl->closeDevice();
}


DSoundOutputDevice::DSoundOutputDevice(GUID deviceId)
:mDevice(NULL), mPrimaryBuffer(NULL), mBuffer(NULL),
mWriteOffset(0), mPlayedSamples(0), mTotalPlayed(0), mTail(0),
mThreadHandle(0), mSimulate(false), mGUID(deviceId),
mNullAudio(AUDIO_SPK_BUFFER_LENGTH, AUDIO_SPK_BUFFER_COUNT)
{
  gDSoundInit.load();
  mShutdownSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mBufferSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mRefCount = 0;
}


DSoundOutputDevice::~DSoundOutputDevice()
{
  close();

  // Destroy used signals
  ::CloseHandle(mShutdownSignal);
  ::CloseHandle(mBufferSignal);
  gDSoundInit.unload();
}

bool DSoundOutputDevice::open()
{
  ICELogInfo(<< "Request to DirectSound audio output");
  // Start thread
  mRefCount++;
  if (mRefCount == 1)
  {
    ICELogInfo(<< "Schedule DirectSound audio output thread");
    mThreadHandle = (HANDLE)_beginthread(&threadProc, 0, this);
    ::SetThreadPriority(mThreadHandle, THREAD_PRIORITY_TIME_CRITICAL);
  }
  return true;
}

void DSoundOutputDevice::close()
{
  if (mRefCount == 0)
    return;
  mRefCount--;
  if (mRefCount > 0)
    return;

  // Tell the thread to exit
  SetEvent(mShutdownSignal);

  // Wait for thread
  if (mThreadHandle)
    WaitForSingleObject(mThreadHandle, INFINITE);
  mThreadHandle = 0;
}

void DSoundOutputDevice::openDevice()
{
  ICELogInfo(<< "Open DirectSound audio output");
  if (IsEqualGUID(mGUID, GUID_NULL))
  {
    setSimulate( true );
    return;
  }
  
  mWriteOffset = 0;
  mPlayedSamples = 0;
  mSentBytes = 0;
  mPlayCursor = 0;
  mBufferSize = AUDIO_SPK_BUFFER_COUNT * AUDIO_SPK_BUFFER_SIZE;

  DSoundHelper::checkComResult(gDSoundInit.mRoutines.DirectSoundCreate8(&mGUID, &mDevice, NULL));
  DSoundHelper::checkComResult(mDevice->SetCooperativeLevel(::GetDesktopWindow(), DSSCL_PRIORITY));

  WAVEFORMATEX wfx;
  memset(&wfx, 0, sizeof(wfx));

  wfx.cbSize = sizeof(wfx);
  wfx.nChannels = AUDIO_CHANNELS;
  wfx.nSamplesPerSec = AUDIO_SAMPLERATE;
  wfx.wBitsPerSample = 16;
  wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
  wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
  wfx.wFormatTag = WAVE_FORMAT_PCM;

  DSBUFFERDESC dsbd;
  ZeroMemory(&dsbd, sizeof(dsbd));
  dsbd.dwSize = sizeof(DSBUFFERDESC);
  dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
  dsbd.dwBufferBytes = 0;
  dsbd.lpwfxFormat = NULL;//&wfx;
  dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;

  DSoundHelper::checkComResult(mDevice->CreateSoundBuffer(&dsbd, &mPrimaryBuffer, NULL ));
  DSBCAPS caps; 
  caps.dwSize = sizeof(caps);
  caps.dwFlags = 0;
  caps.dwBufferBytes = 0;
  caps.dwPlayCpuOverhead = 0;
  caps.dwUnlockTransferRate = 0;
  
  DSoundHelper::checkComResult(mPrimaryBuffer->GetCaps(&caps));

  dsbd.dwSize = sizeof(caps);
  dsbd.dwFlags = DSBCAPS_GLOBALFOCUS;
  dsbd.lpwfxFormat = &wfx;
  dsbd.guid3DAlgorithm = DS3DALG_DEFAULT;
  dsbd.dwBufferBytes = mBufferSize;
  
  DSoundHelper::checkComResult(mDevice->CreateSoundBuffer(&dsbd, &mBuffer, NULL));
  
  // Fill the buffer with silence
  LPVOID ptr1 = NULL, ptr2 = NULL; DWORD len1 = 0, len2 = 0;
  DSoundHelper::checkComResult(mBuffer->Lock(0, AUDIO_SPK_BUFFER_SIZE * AUDIO_SPK_BUFFER_COUNT, &ptr1, &len1, &ptr2, &len2, 0));
  if (len1 && ptr1)
    memset(ptr1, 0, len1);
  if (len2 && ptr2)
    memset(ptr2, 0, len2);
  DSoundHelper::checkComResult(mBuffer->Unlock(ptr1, len1, ptr2, len2));
  DSoundHelper::checkComResult(mBuffer->Play(0,0,DSBPLAY_LOOPING));
  mBuffer->GetCurrentPosition(NULL, &mWriteCursor);
}

void DSoundOutputDevice::closeDevice()
{
  if (mBuffer)        
  { 
    mBuffer->Stop(); 
    mBuffer->Release(); 
    mBuffer = NULL; 
  }
  
  if (mPrimaryBuffer) 
  { 
    mPrimaryBuffer->Stop(); 
    mPrimaryBuffer->Release(); 
    mPrimaryBuffer = NULL; 
  }
  
  if (mDevice)        
  { 
    mDevice->Release(); 
    mDevice = NULL; 
  }
}

void DSoundOutputDevice::restoreBuffer()
{
  if (mSimulate)
    return;

  DWORD status = 0;
  DSoundHelper::checkComResult(mBuffer->GetStatus(&status));
  if (DSBSTATUS_BUFFERLOST == status)
    DSoundHelper::checkComResult(mBuffer->Restore());
}

bool DSoundOutputDevice::getMediaFrame()
{
  try
  {
    memset(mMediaFrame, 0, sizeof mMediaFrame);
    if (mConnection)
      mConnection->onSpkData(getFormat(), mMediaFrame, sizeof mMediaFrame);
  }
  catch(...)
  {}

  return true;
}

bool DSoundOutputDevice::process()
{
  if (mSimulate)
    return false;

  // Find amount of written data from last call
  DWORD cursor = 0;
  DSoundHelper::checkComResult(mBuffer->GetCurrentPosition(NULL, &cursor));
  unsigned written;
  if (cursor < mWriteCursor)
    written = mBufferSize - mWriteCursor + cursor;
  else
    written = cursor - mWriteCursor;
  
  mWriteCursor += (written / AUDIO_SPK_BUFFER_SIZE) * AUDIO_SPK_BUFFER_SIZE;
  mWriteCursor %= mBufferSize;
  bool finished = false;
  for (unsigned frameIndex = 0; frameIndex < written / AUDIO_SPK_BUFFER_SIZE && !finished; frameIndex++)
  {
    unsigned offset = mWriteOffset + frameIndex * AUDIO_SPK_BUFFER_SIZE;
    offset %= mBufferSize;

    // See what we can write
    LPVOID ptr1 = NULL, ptr2 = NULL; DWORD len1 = 0, len2 = 0;
    DSoundHelper::checkComResult(mBuffer->Lock(offset, AUDIO_SPK_BUFFER_SIZE, &ptr1, &len1, &ptr2, &len2, 0));
    
    assert(ptr2 == NULL);
    assert(len1 >= AUDIO_SPK_BUFFER_SIZE);
    
    if (getMediaFrame())
      finished = true;
    memmove(ptr1, mMediaFrame, AUDIO_SPK_BUFFER_SIZE);
    DSoundHelper::checkComResult(mBuffer->Unlock(ptr1, AUDIO_SPK_BUFFER_SIZE, ptr2, 0));
  }

  // Increase write offset
  mWriteOffset += (written / AUDIO_SPK_BUFFER_SIZE) * AUDIO_SPK_BUFFER_SIZE;
  mWriteOffset %= mBufferSize;

  return true;
}


void DSoundOutputDevice::threadProc(void* arg)
{
  DSoundOutputDevice* impl = (DSoundOutputDevice*)arg;
  impl->openDevice();
  
  DWORD waitResult = 0;
  HANDLE waitArray[2] = {impl->mBufferSignal, impl->mShutdownSignal};
  unsigned exitCount = 0;
  bool exitSignal = false;
  while (true)
  {
    // Poll for shutdown signal
    if (WAIT_OBJECT_0 == ::WaitForSingleObject(impl->mShutdownSignal, 0))
      break;
    
    if (impl->isSimulate())
    {
      impl->mNullAudio.waitForBuffer();
      impl->getMediaFrame();
    }
    else
    {
      // Poll events
      waitResult = ::WaitForMultipleObjects(2, waitArray, FALSE, 5);
      if (waitResult == WAIT_OBJECT_0 + 1)
        break;
      try
      {
        impl->restoreBuffer();
        impl->process();
      }
      catch(const Exception& e)
      {
        ICELogCritical(<< "DirectSound output failed with code = " << e.code() << ", subcode = " << e.subcode());
        impl->setSimulate(true);
      }
      catch(...)
      {
        ICELogCritical(<< "DirectSound output failed due to unexpected exception.");
        impl->setSimulate(true);
      }
    }
  }
  impl->closeDevice();
}

unsigned DSoundOutputDevice::playedTime() const
{
  return 0;
}


void DSoundOutputDevice::setSimulate(bool s)
{
  mSimulate = s;
}

bool DSoundOutputDevice::isSimulate() const
{
  return mSimulate;
}

Format DSoundOutputDevice::getFormat()
{
  return Format();
}

bool DSoundOutputDevice::closing()
{
  return false;
}

typedef WINUSERAPI HRESULT (WINAPI *LPFNDLLGETCLASSOBJECT) (const CLSID &, const IID &, void **);

HRESULT DirectSoundPrivateCreate (OUT LPKSPROPERTYSET * ppKsPropertySet) 
{ 
  HMODULE                 hLibDsound              = NULL; 
  LPFNDLLGETCLASSOBJECT   pfnDllGetClassObject    = NULL; 
  LPCLASSFACTORY          pClassFactory           = NULL; 
  LPKSPROPERTYSET         pKsPropertySet          = NULL; 
  HRESULT                 hr                      = DS_OK; 

  // Load dsound.dll 
  hLibDsound = LoadLibrary(TEXT("dsound.dll")); 

  if(!hLibDsound) 
  { 
    hr = DSERR_GENERIC; 
  } 

  // Find DllGetClassObject 
  if(SUCCEEDED(hr)) 
  { 
    pfnDllGetClassObject = 
      (LPFNDLLGETCLASSOBJECT)GetProcAddress ( hLibDsound, "DllGetClassObject" ); 


    if(!pfnDllGetClassObject) 
    { 
      hr = DSERR_GENERIC; 
    } 
  } 

  // Create a class factory object     
  if(SUCCEEDED(hr)) 
  { 
    hr = pfnDllGetClassObject (CLSID_DirectSoundPrivate, IID_IClassFactory, (LPVOID *)&pClassFactory ); 
  } 

  // Create the DirectSoundPrivate object and query for an IKsPropertySet 
  // interface 
  if(SUCCEEDED(hr)) 
  { 
    hr = pClassFactory->CreateInstance ( NULL, IID_IKsPropertySet, (LPVOID *)&pKsPropertySet ); 
  } 

  // Release the class factory 
  if(pClassFactory) 
  { 
    pClassFactory->Release(); 
  } 

  // Handle final success or failure 
  if(SUCCEEDED(hr)) 
  { 
    *ppKsPropertySet = pKsPropertySet; 
  } 
  else if(pKsPropertySet) 
  { 
    pKsPropertySet->Release(); 
  } 

  FreeLibrary(hLibDsound); 

  return hr; 
} 

BOOL GetInfoFromDSoundGUID( GUID i_sGUID, int &dwWaveID)
{ 
  LPKSPROPERTYSET         pKsPropertySet = NULL; 
  HRESULT                 hr; 
  BOOL					retval = FALSE;

  PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA psDirectSoundDeviceDescription = NULL; 
  DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA sDirectSoundDeviceDescription;

  memset(&sDirectSoundDeviceDescription,0,sizeof(sDirectSoundDeviceDescription)); 
  hr = DirectSoundPrivateCreate( &pKsPropertySet ); 
  if(SUCCEEDED(hr)) 
  { 
    ULONG ulBytesReturned = 0;
    sDirectSoundDeviceDescription.DeviceId = i_sGUID; 

    // On the first call the final size is unknown so pass the size of the struct in order to receive
    // "Type" and "DataFlow" values, ulBytesReturned will be populated with bytes required for struct+strings.
    hr = pKsPropertySet->Get(DSPROPSETID_DirectSoundDevice, 
      DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION, 
      NULL, 
      0, 
      &sDirectSoundDeviceDescription, 
      sizeof(sDirectSoundDeviceDescription), 
      &ulBytesReturned
      ); 

    if (ulBytesReturned)
    {
      // On the first call it notifies us of the required amount of memory in order to receive the strings.
      // Allocate the required memory, the strings will be pointed to the memory space directly after the struct.
      psDirectSoundDeviceDescription = (PDSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION_DATA)new BYTE[ulBytesReturned];
      *psDirectSoundDeviceDescription = sDirectSoundDeviceDescription;

      hr = pKsPropertySet->Get(DSPROPSETID_DirectSoundDevice, 
        DSPROPERTY_DIRECTSOUNDDEVICE_DESCRIPTION, 
        NULL, 
        0, 
        psDirectSoundDeviceDescription, 
        ulBytesReturned, 
        &ulBytesReturned
        ); 

      dwWaveID  = psDirectSoundDeviceDescription->WaveDeviceId;
      /*Description = psDirectSoundDeviceDescription->Description;
      Module = psDirectSoundDeviceDescription->Module;
      Interface = psDirectSoundDeviceDescription->Interface;*/
      delete [] psDirectSoundDeviceDescription;
      retval = TRUE;
    }

    pKsPropertySet->Release(); 
  } 

  return retval; 
} 

struct EnumResult
{
  int mDeviceId;
  GUID mGuid;
};

BOOL CALLBACK DSEnumCallback(
  LPGUID  lpGuid,    
  LPCTSTR  lpcstrDescription,  
  LPCTSTR  lpcstrModule,   
  LPVOID  lpContext    
  )
{
  if (lpGuid)
  {

    int devId = -1;  
    GetInfoFromDSoundGUID(*lpGuid, devId);
    EnumResult* er = (EnumResult*)lpContext;
    if (er->mDeviceId == devId)
    {
      er->mGuid = *lpGuid;
      return FALSE;
    }
    else
      return TRUE;
  }
  else
    return TRUE;
}


GUID DSoundHelper::deviceId2Guid(int deviceId, bool captureDevice)
{
  EnumResult er;
  er.mDeviceId = deviceId;
  er.mGuid = GUID_NULL;
  memset(&er.mGuid, 0, sizeof er.mGuid);
  if (captureDevice)
    DirectSoundCaptureEnumerate(DSEnumCallback, &er);
  else
    DirectSoundEnumerate(DSEnumCallback, &er);

  return er.mGuid;
}

void DSoundHelper::checkComResult(HRESULT code)
{
    if (FAILED(code))
      throw Exception(ERR_DSOUND);
}

#endif
