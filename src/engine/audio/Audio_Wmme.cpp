/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef TARGET_WIN

#include "Audio_Wmme.h"
#include "Audio_Helper.h"
#include "../Helper/HL_Exception.h"

#include <process.h>
using namespace Audio;


WmmeInputDevice::Buffer::Buffer()
{
  // Do not use WAVEHDR allocated on stack!
  mHeaderHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, sizeof WAVEHDR);
  if (!mHeaderHandle)
    throw Exception(ERR_WMME_FAILED, GetLastError());
  mHeader = (WAVEHDR*)GlobalLock(mHeaderHandle);

  mDataHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, AUDIO_MIC_BUFFER_SIZE);
  if (!mDataHandle)
    throw Exception(ERR_WMME_FAILED, GetLastError());
  mData = GlobalLock(mDataHandle);
  
  memset(mHeader, 0, sizeof *mHeader);
  mHeader->dwBufferLength = AUDIO_MIC_BUFFER_SIZE;
  mHeader->dwFlags = 0;
  mHeader->lpData = (LPSTR)mData;
}

WmmeInputDevice::Buffer::~Buffer()
{
  if (mDataHandle)
  {
    GlobalUnlock(mDataHandle); 
    GlobalFree(mDataHandle);
  }
  if (mHeaderHandle)
  {
    GlobalUnlock(mHeaderHandle);
    GlobalFree(mHeaderHandle);
  }
  
}

bool WmmeInputDevice::Buffer::prepare(HWAVEIN device)
{
  MMRESULT resCode = MMSYSERR_NOERROR;
  mHeader->dwFlags = 0;
  mHeader->dwBufferLength = AUDIO_MIC_BUFFER_SIZE;
  mHeader->lpData = (LPSTR)mData;

  resCode = waveInPrepareHeader(device, mHeader, sizeof *mHeader);
  //if (resCode != MMSYSERR_NOERROR)
  //  LogCritical("Audio", << "Failed to prepare source header. Error code " << resCode << ".");

  return resCode == MMSYSERR_NOERROR;
}

bool WmmeInputDevice::Buffer::unprepare(HWAVEIN device)
{
  if (mHeader->dwFlags & WHDR_PREPARED)
  {
    MMRESULT resCode = waveInUnprepareHeader(device, mHeader, sizeof *mHeader);
    //if (resCode != MMSYSERR_NOERROR)
    // LogCritical("Audio", << "Failed to unprepare source header. Error code " << resCode << ".");
    return resCode == MMSYSERR_NOERROR;
  }
  return true;
}

bool WmmeInputDevice::Buffer::isFinished()
{
  return (mHeader->dwFlags & WHDR_DONE) != 0;
}

bool WmmeInputDevice::Buffer::addToDevice(HWAVEIN device)
{
  MMRESULT resCode = waveInAddBuffer(device, mHeader, sizeof(*mHeader));
  //if (resCode != MMSYSERR_NOERROR)
  //  LogCritical("Audio", << "Failed to add buffer to source audio device. Error code is " << resCode << ".");
  return resCode == MMSYSERR_NOERROR;
}

void* WmmeInputDevice::Buffer::data()
{
  return mData;
}


WmmeInputDevice::WmmeInputDevice(int deviceId)
:mDevHandle(NULL), mDoneSignal(INVALID_HANDLE_VALUE), mFakeMode(false),
mBufferIndex(0), mDeviceIndex(deviceId), mThreadHandle(0)
{
  mDoneSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mShutdownSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mRefCount = 0;
}

WmmeInputDevice::~WmmeInputDevice()
{
  close();
  ::CloseHandle(mDoneSignal);
  ::CloseHandle(mShutdownSignal);
}

bool WmmeInputDevice::fakeMode()
{
  return mFakeMode;
}

void CALLBACK WmmeInputDevice::callbackProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
  WmmeInputDevice* impl;
  switch(uMsg)
  {
  case WIM_DATA:
    impl = (WmmeInputDevice*)dwInstance;
    SetEvent(impl->mDoneSignal);
    break;

  case WIM_CLOSE:
    break;

  case WIM_OPEN:
    break;
  }
}

void WmmeInputDevice::openDevice()
{
  // Build WAVEFORMATEX structure
  WAVEFORMATEX wfx;
  memset(&wfx, 0, sizeof(wfx));

  wfx.wFormatTag = WAVE_FORMAT_PCM;
  wfx.nChannels = AUDIO_CHANNELS;
  wfx.nSamplesPerSec = AUDIO_SAMPLERATE;
  wfx.wBitsPerSample = 16;
  wfx.cbSize = 0;
  wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
  wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;
  
  // Open wavein
  
  MMRESULT mmres = waveInOpen(&mDevHandle, mDeviceIndex, &wfx, (DWORD_PTR)callbackProc, (DWORD_PTR)this,  CALLBACK_FUNCTION);
  if (mmres != MMSYSERR_NOERROR)
  {
    mFakeMode = true;
    return;
  }
  else
    mFakeMode = false;

  // Create the buffers for running
  mBufferIndex = 0;
  for (int i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
    mBufferList[i].prepare(mDevHandle);
    
  for (int i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
    mBufferList[i].addToDevice(mDevHandle);

  /*mmres = */waveInStart(mDevHandle);
}

bool WmmeInputDevice::open()
{
  Lock lock(mGuard);
  
  mRefCount++;
  if (mRefCount > 1)
    return true;
  
  mThreadHandle = (HANDLE)_beginthread(&threadProc, 0, this);
  return true;
}

void WmmeInputDevice::closeDevice()
{
  // Stop device
  if (mDevHandle)
  {
	  MMRESULT mmres = MMSYSERR_NOERROR;
    waveInReset(mDevHandle); 
    waveInStop(mDevHandle);
  }    
  
  // Close buffers
  for (int i=0; i<AUDIO_MIC_BUFFER_COUNT; i++)
    mBufferList[i].unprepare(mDevHandle);
  
  // Close device
  if (mDevHandle)
  {
    waveInClose(mDevHandle); 
    mDevHandle = NULL;
  }
}

void WmmeInputDevice::close()
{
  Lock l(mGuard);

  mRefCount--;
  if (mRefCount != 0)
    return;
  
  // Set shutdown signal
  if (!mThreadHandle)
    return;

  ::SetEvent(mShutdownSignal);
  ::WaitForSingleObject(mThreadHandle, INFINITE);
  mThreadHandle = 0;
  
}

bool WmmeInputDevice::tryReadBuffer(void* buffer)
{
  Buffer& devBuffer = mBufferList[mBufferIndex];

  if (!devBuffer.isFinished())
    return false;
  memcpy(buffer, devBuffer.data(), AUDIO_MIC_BUFFER_SIZE);
  devBuffer.unprepare(mDevHandle);
  devBuffer.prepare(mDevHandle);
  if (!devBuffer.addToDevice(mDevHandle))
    setFakeMode(true);
  else
  {
  }
  mBufferIndex = (mBufferIndex + 1) % AUDIO_MIC_BUFFER_COUNT;
  return true;
}

void WmmeInputDevice::setFakeMode(bool fakeMode)
{
  mFakeMode = fakeMode;
}

int WmmeInputDevice::readBuffer(void* buffer)
{
  //Lock lock(mGuard);

  if (mRefCount <= 0 || mFakeMode)
    return 0;
  
  // Check for finished buffer
  while (!tryReadBuffer(buffer))
    WaitForSingleObject(mDoneSignal, 50);
  
  return AUDIO_MIC_BUFFER_SIZE;
}

HWAVEIN WmmeInputDevice::handle()
{
  Lock lock(mGuard);
  return mDevHandle;
}

void WmmeInputDevice::threadProc(void* arg)
{
  WmmeInputDevice* impl = (WmmeInputDevice*)arg;
  impl->openDevice();
  void* buffer = _alloca(AUDIO_MIC_BUFFER_SIZE);

  DWORD waitResult = 0;
  HANDLE waitArray[2] = {impl->mDoneSignal, impl->mShutdownSignal};
  DWORD wr;
  do
  {
    wr = ::WaitForMultipleObjects(2, waitArray, FALSE, INFINITE);
    
    if (wr == WAIT_OBJECT_0)
    {
      impl->readBuffer(buffer);
      if (impl->connection())
        impl->connection()->onMicData(Format(), buffer, AUDIO_MIC_BUFFER_SIZE);
    }
  } while (wr == WAIT_OBJECT_0);

  impl->closeDevice();
}

// --- WmmeOutputDevice ---
WmmeOutputDevice::Buffer::Buffer()
:mHeaderHandle(NULL), mDataHandle(NULL), mData(NULL), mHeader(NULL)
{
  mHeaderHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, AUDIO_SPK_BUFFER_SIZE);
  if (!mHeaderHandle)
    throw Exception(ERR_NOMEM);

  mDataHandle = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, AUDIO_SPK_BUFFER_SIZE);
  if (!mDataHandle)
    throw Exception(ERR_NOMEM);

  mHeader = (WAVEHDR*)GlobalLock(mHeaderHandle);
  mData = GlobalLock(mDataHandle);
  memset(mHeader, 0, sizeof *mHeader);
  mHeader->dwBufferLength = AUDIO_SPK_BUFFER_SIZE;
  mHeader->lpData = (LPSTR)mData;
}

WmmeOutputDevice::Buffer::~Buffer()
{
  if (mHeaderHandle)
  {
    GlobalUnlock(mHeaderHandle);
    GlobalFree(mHeaderHandle);
  }
  if (mDataHandle)
  {
    GlobalUnlock(mDataHandle);
    GlobalFree(mDataHandle);
  }
}

bool WmmeOutputDevice::Buffer::prepare(HWAVEOUT device)
{
  MMRESULT result;
  result = ::waveOutPrepareHeader(device, mHeader, sizeof *mHeader);
  return result == MMSYSERR_NOERROR;
}

bool WmmeOutputDevice::Buffer::unprepare(HWAVEOUT device)
{
  MMRESULT result;
  result = ::waveOutUnprepareHeader(device, mHeader, sizeof *mHeader);
  return result == MMSYSERR_NOERROR;
}

bool WmmeOutputDevice::Buffer::write(HWAVEOUT device)
{
  MMRESULT result;
  result = ::waveOutWrite(device, mHeader, sizeof *mHeader);
  return result == MMSYSERR_NOERROR;
}

WmmeOutputDevice::WmmeOutputDevice(int index)
:mDevice(NULL), mDeviceIndex(index), mPlayedTime(0), mPlayedCount(0), mBufferIndex(0), mThreadHandle(NULL), 
mFailed(false), mShutdownMarker(false)
{
  mDoneSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
  mShutdownSignal = ::CreateEvent(NULL, FALSE, FALSE, NULL);
}


WmmeOutputDevice::~WmmeOutputDevice()
{
  close();

  // Destroy used signals
  CloseHandle(mDoneSignal); CloseHandle(mShutdownSignal);
}

bool WmmeOutputDevice::open()
{
  // Start thread
  mThreadHandle = (HANDLE)_beginthread(&threadProc, 0, this);
  return true;
}

void WmmeOutputDevice::close()
{
  // Tell the thread to exit
  SetEvent(mShutdownSignal);
  mShutdownMarker = true;

  // Wait for thread
  if (mThreadHandle)
    WaitForSingleObject(mThreadHandle, INFINITE);
  mThreadHandle = 0;
}

void WmmeOutputDevice::openDevice()
{
  mClosing = false;
  MMRESULT mmres = 0;
  WAVEFORMATEX wfx;
  memset(&wfx, 0, sizeof(wfx));
  wfx.wFormatTag = 0x0001;
  wfx.nChannels = AUDIO_CHANNELS;
  wfx.nSamplesPerSec = AUDIO_SAMPLERATE;
  wfx.wBitsPerSample = 16;
  wfx.cbSize = 0;
  wfx.nBlockAlign = wfx.wBitsPerSample * wfx.nChannels / 8;
  wfx.nAvgBytesPerSec = wfx.nBlockAlign * wfx.nSamplesPerSec;

	mmres = waveOutOpen(&mDevice, mDeviceIndex, &wfx, (DWORD_PTR)&callbackProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
  if (mmres != MMSYSERR_NOERROR)
    throw Exception(ERR_WMME_FAILED, mmres);

  // Prebuffer silence
  for (unsigned i=0; i<AUDIO_SPK_BUFFER_COUNT; i++)
  {
    //bool dumb = false;
    //mCallback(mBufferList[i].mData, SPK_BUFFER_SIZE, dumb, dumb);
    memset(mBufferList[i].mData, 0, AUDIO_SPK_BUFFER_SIZE);
    mBufferList[i].prepare(mDevice);
    mBufferList[i].write(mDevice);
  }
}

void WmmeOutputDevice::closeDevice()
{
  Lock l(mGuard);

  mClosing = true;
  bool finished = false;
  while (!finished)
  {
    WaitForSingleObject(mDoneSignal, 10);
    finished = areBuffersFinished();
  }

  if (mDevice)
  {
    waveOutReset(mDevice);
    waveOutClose(mDevice);
  }
  
  mDevice = NULL;
}

bool WmmeOutputDevice::areBuffersFinished()
{
  Lock l(mGuard);
  bool result = true;
  for (unsigned i=0; i<AUDIO_SPK_BUFFER_COUNT && result; i++)
  {
    bool finished = mBufferList[i].mHeader->dwFlags & WHDR_DONE || 
                    !mBufferList[i].mHeader->dwFlags;
    if (finished)
    {
/*      if (mBufferList[i].mHeader->dwFlags & WHDR_PREPARED)
        mBufferList[i].Unprepare(mDevice); */
    }
    result &= finished;
  }

  return result;
}

void WmmeOutputDevice::threadProc(void* arg)
{
  WmmeOutputDevice* impl = (WmmeOutputDevice*)arg;
  impl->openDevice();

  DWORD waitResult = 0;
  HANDLE waitArray[2] = {impl->mDoneSignal, impl->mShutdownSignal};
  unsigned index, i;
  unsigned exitCount = 0;
  bool exitSignal = false;
  do
  {
    // Poll for exit signal
    if (!exitSignal)
      exitSignal = impl->mShutdownMarker;
    
    // Wait for played buffer
    WaitForSingleObject(impl->mDoneSignal, 500);
    
    // Iterate buffers to find played
    for (i=0; i<AUDIO_SPK_BUFFER_COUNT; i++)
    {
      index = (impl->mBufferIndex + i) % AUDIO_SPK_BUFFER_COUNT;
      Buffer& buffer = impl->mBufferList[index];
      if (!(buffer.mHeader->dwFlags & WHDR_DONE))
        break;
      
      buffer.unprepare(impl->mDevice);
      if (!exitSignal)
      {
        bool useAEC = true;
        if (impl->connection())
          impl->connection()->onSpkData(Format(), buffer.mData, AUDIO_SPK_BUFFER_SIZE);
        else
          memset(buffer.mData, 0, AUDIO_SPK_BUFFER_SIZE);
        
        buffer.prepare(impl->mDevice);
        buffer.write(impl->mDevice);
      }
      else
        exitCount++;
    }
    impl->mBufferIndex = (impl->mBufferIndex + i) % AUDIO_SPK_BUFFER_COUNT;
  }
  while (!exitSignal || exitCount < AUDIO_SPK_BUFFER_COUNT);
  impl->closeDevice();
}

HWAVEOUT WmmeOutputDevice::handle()
{
  return mDevice;
}

unsigned WmmeOutputDevice::playedTime()
{
  if (!mDevice)
    return 0;
  unsigned result = 0;

  MMTIME mmt; 
  memset(&mmt, 0, sizeof(mmt));
  mmt.wType = TIME_SAMPLES;
  MMRESULT rescode = waveOutGetPosition(mDevice, &mmt, sizeof(mmt));
  if (rescode != MMSYSERR_NOERROR || mmt.wType != TIME_SAMPLES)
    closeDevice();
  else
  {
    if (mmt.u.ms < mPlayedTime)
      result = 0;
    else
    {
      result = mmt.u.ms - mPlayedTime;
      mPlayedTime = mmt.u.ms - result % 8;
    }
  }

  return result / 8;
}

void WmmeOutputDevice::setFakeMode(bool fakemode)
{
  closeDevice();
}

bool WmmeOutputDevice::fakeMode()
{
  return mFailed;
}


bool WmmeOutputDevice::closing()
{
  return mClosing;
}

void CALLBACK WmmeOutputDevice::callbackProc(HWAVEOUT hwo, UINT msg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
  WmmeOutputDevice* impl;

  if (msg == WOM_DONE)
  {
    impl = (WmmeOutputDevice*)dwInstance;
    InterlockedIncrement(&impl->mPlayedCount);
    SetEvent(impl->mDoneSignal);
  }
}

#endif
