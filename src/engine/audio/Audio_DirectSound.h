/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_DSOUND_H
#define __AUDIO_DSOUND_H

#include "../config.h"

#include <winsock2.h>
#include <windows.h>
#include <mmsystem.h>

#include "../Helper/HL_Sync.h"
#include "../Helper/HL_ByteBuffer.h"
#include "Audio_WavFile.h"
#include "Audio_Interface.h"
#include "Audio_Helper.h"

#include <deque>
#include <EndpointVolume.h>
#include <MMDeviceAPI.h>
#if defined(_MSC_VER)
# include <Functiondiscoverykeys_devpkey.h>
#endif
#include <vector>
#include <string>
#include <InitGuid.h>
#include <dsound.h>

namespace Audio
{
  class VistaEnumerator: public Enumerator
  {
  public:
    VistaEnumerator();
    ~VistaEnumerator();

    void open(int direction);
    void close();

    int count();
    std::tstring nameAt(int index);
    int idAt(int index);
    int indexOfDefaultDevice();

  protected:
    IMMDeviceCollection*        mCollection;
    IMMDevice*                  mDefaultDevice;
    IMMDeviceEnumerator*        mEnumerator;
    EDataFlow                   mDirection;
    std::vector<std::wstring>   mNameList;

    void enumerate();
    IMMDevice* mapIndexToInterface(int index);
  };

  class XpEnumerator: public Enumerator
  {
  public:
    XpEnumerator();
    ~XpEnumerator();

    void open(int direction);
    void close();

    int count();
    std::tstring nameAt(int index);
    int idAt(int index);
    int indexOfDefaultDevice();

  protected:
    std::vector<std::wstring> mNameList;
    int mDirection;
  };

  class DSoundHelper
  {
  public:
    static void checkComResult(HRESULT code);
    static GUID deviceId2Guid(int deviceId, bool captureDevice);
  };

#if !defined(_MSC_VER)
  typedef struct IDirectSoundNotify8          *LPDIRECTSOUNDNOTIFY8;
#endif

  class DSoundInputDevice: public InputDevice
  {
  public:
    DSoundInputDevice(GUID deviceId);
    ~DSoundInputDevice();
    
    void enableDenoiser(bool enable);
    bool open();
    void close();
    
    bool isSimulate() const;
    void setSimulate(bool s);
    
    int readBuffer(void* buffer);
    Format getFormat();

  protected:
    Mutex         mGuard;                         /// Mutex to protect this instance.
    LPDIRECTSOUNDCAPTURE8         mDevice;
    LPDIRECTSOUNDCAPTUREBUFFER8   mBuffer;
    LPDIRECTSOUNDNOTIFY8          mNotifications;
    DSBPOSITIONNOTIFY             mEventArray[AUDIO_MIC_BUFFER_COUNT];
    HANDLE                        mEventSignals[AUDIO_MIC_BUFFER_COUNT]; // Helper array to make WaitForMultipleObjects in loop

    int           mBufferIndex;
    int           mNextBuffer;
    GUID          mGUID;

    HANDLE        mThreadHandle;
    HANDLE        mShutdownSignal;
    volatile bool mSimulate;                           /// Marks if simulate mode is active.
    int           mRefCount;
    ByteBuffer    mQueue;
    unsigned      mReadOffset;
    DenoiseFilter mDenoiser;
    volatile bool mEnableDenoiser;
    char          mTempBuffer[AUDIO_MIC_BUFFER_SIZE];
    StubTimer     mNullAudio;

#ifdef AUDIO_DUMPINPUT
    WavFileWriter mDump;
#endif

    bool          tryReadBuffer(void* buffer);
    void          openDevice();
    void          closeDevice();

    static void threadProc(void* arg);
  };

  class DSoundOutputDevice: public OutputDevice
  {
  public:
    DSoundOutputDevice(GUID deviceId);
    ~DSoundOutputDevice();
    
    bool open();
    void close();

    unsigned      playedTime() const;
    bool          isSimulate() const;
    void          setSimulate(bool s);
    bool          closing();
    Format        getFormat();

  protected:
    Mutex                       mGuard;               /// Mutex to protect this instance
    int                         mDeviceID;
    LPDIRECTSOUND8              mDevice;
    LPDIRECTSOUNDBUFFER         mPrimaryBuffer;
    LPDIRECTSOUNDBUFFER         mBuffer;
    GUID                        mGUID;
    unsigned                    mWriteOffset;
    unsigned                    mPlayedSamples;
    unsigned                    mSentBytes;
    DWORD                       mPlayCursor;  // Measured in bytes
    unsigned                    mBufferSize;
    unsigned                    mTotalPlayed; // Measured in bytes
    unsigned                    mTail;        // Measured in bytes
    HANDLE                      mShutdownSignal;
    HANDLE                      mBufferSignal;
    HANDLE                      mThreadHandle;
    bool                        mSimulate;
    StubTimer                   mNullAudio;
    DWORD                       mWriteCursor;
    char                        mMediaFrame[AUDIO_SPK_BUFFER_SIZE];
    unsigned                    mRefCount;

    void openDevice();
    void closeDevice();
    void restoreBuffer();
    bool process();
    bool getMediaFrame();

    static void threadProc(void* arg);
  };
}

#endif
