/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_WMME_H
#define __AUDIO_WMME_H

#ifdef TARGET_WIN

#include "../engine_config.h"

#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>

#include "../Helper/HL_Sync.h"
#include "Audio_Interface.h"

#include <deque>
#include <EndpointVolume.h>
#include <MMDeviceAPI.h>
#if defined(_MSC_VER)
#include <Functiondiscoverykeys_devpkey.h>
#endif
#include <vector>
#include <string>


namespace Audio
{

  class WmmeInputDevice: public InputDevice
  {
  public:
    WmmeInputDevice(int index);
    ~WmmeInputDevice();
    
    bool open();
    void close();
    
    bool fakeMode();
    void setFakeMode(bool fakeMode);
    
    int readBuffer(void* buffer);
    HWAVEIN handle();

  protected:
    class Buffer
    {
    public:
      Buffer();
      ~Buffer();
      bool    prepare(HWAVEIN device);
      bool    unprepare(HWAVEIN device);
      bool    isFinished();
      bool    addToDevice(HWAVEIN device);
      void*   data();

    protected:
      HGLOBAL       mDataHandle;      
      void*         mData;            
      HGLOBAL       mHeaderHandle;    
      WAVEHDR*      mHeader;          
    };

    Mutex         mGuard;                         /// Mutex to protect this instance.
    HWAVEIN       mDevHandle;                     /// Handle of opened capture device.
    HANDLE        mThreadHandle;
    HANDLE        mShutdownSignal;
    HANDLE        mDoneSignal;                    /// Event handle to signal about finished capture.
    Buffer        mBufferList[AUDIO_MIC_BUFFER_COUNT]; 
    unsigned      mBufferIndex;
    int           mDeviceIndex;                   /// Index of capture device.
    volatile bool mFakeMode;                      /// Marks if fake mode is active.
    int           mRefCount;

    bool          tryReadBuffer(void* buffer);
    void          openDevice();
    void          closeDevice();

    static void CALLBACK callbackProc(HWAVEIN hwi, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    static void threadProc(void* arg);
  };

  class WmmeOutputDevice: public OutputDevice
  {
  public:
    WmmeOutputDevice(int index);
    ~WmmeOutputDevice();
    
    bool open();
    void close();

    HWAVEOUT      handle();
    unsigned      playedTime();
    void          setFakeMode(bool fakemode);
    bool          fakeMode();
    bool          closing();

  protected:
    class Buffer
    {
    friend class WmmeOutputDevice;
    public:
      Buffer();
      ~Buffer();
      bool prepare(HWAVEOUT device);
      bool unprepare(HWAVEOUT device);
      bool write(HWAVEOUT device);  
    protected:
      WAVEHDR*    mHeader;
      void*       mData;
      HGLOBAL     mHeaderHandle;
      HGLOBAL     mDataHandle;
    };

    Mutex                   mGuard;               /// Mutex to protect this instance
    int                     mDeviceIndex;
    HWAVEOUT                mDevice;              /// Handle of opened audio device
    Buffer                  mBufferList[AUDIO_SPK_BUFFER_COUNT];
    unsigned                mPlayedTime;          /// Amount of played time in milliseconds
    bool                    mClosing;
    HANDLE                  mDoneSignal,
                            mShutdownSignal,
                            mThreadHandle;
    volatile bool           mShutdownMarker;
  
    volatile LONG           mPlayedCount;
    unsigned                mBufferIndex;
    bool                    mFailed;

    void openDevice();
    void closeDevice();
    bool areBuffersFinished();

    static void CALLBACK callbackProc(HWAVEOUT hwo, UINT msg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
    static void threadProc(void* arg);
  };




}

#endif

#endif
