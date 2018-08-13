/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __AUDIO_COREAUDIO_H
#define __AUDIO_COREAUDIO_H

#ifdef TARGET_OSX

#include "Audio_Interface.h"
#include "Audio_Helper.h"
#include "Audio_Resampler.h"
#include "Audio_DataWindow.h"
#include "../helper/HL_Pointer.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Exception.h"
#include <AudioToolbox/AudioQueue.h>
#include <memory>

// Define CoreAudio buffer time length in milliseconds
#define COREAUDIO_BUFFER_TIME 20

namespace Audio
{

class AudioException: public Exception
{
public:
  AudioException(int code, OSStatus subcode)
    :Exception(code, int(subcode))
  {}
};

//#ifndef AudioDeviceID
//#   define AudioDeviceID unsigned
//#endif
class MacEnumerator: public Enumerator
{
public:
  MacEnumerator();
  ~MacEnumerator();

  void open(int direction);
  void close();

  int count();
  std::tstring nameAt(int index);
  int idAt(int index);
  int indexOfDefaultDevice();

protected:
  struct DeviceInfo
  {
    AudioDeviceID mId;
    std::string mName;
    bool mCanChangeOutputVolume;
    bool mCanChangeInputVolume;
    int mInputCount, mOutputCount;
    int mDefaultRate;
    DeviceInfo(): mId(0), mCanChangeOutputVolume(false), mCanChangeInputVolume(false), mInputCount(0), mOutputCount(0), mDefaultRate(16000) {}
  };
  std::vector<DeviceInfo> mDeviceList;
  unsigned mDefaultInput, mDefaultOutput;
  int mDirection;
  void getInfo(DeviceInfo& di);
};

class CoreAudioUnit
{
public:
  CoreAudioUnit();
  ~CoreAudioUnit();

  void open(bool voice);
  void close();
  AudioStreamBasicDescription getFormat(int scope, int bus);
  void setFormat(AudioStreamBasicDescription& format, int scope, int bus);
  bool getEnabled(int scope, int bus);
  void setEnabled(bool enabled, int scope, int bus);
  void makeCurrent(AudioDeviceID deviceId, int scope, int bus);
  void setCallback(AURenderCallbackStruct cb, int callbackType, int scope, int bus);
  void setBufferFrameSizeInMilliseconds(int ms);
  int getBufferFrameSize();
  void initialize();
  AudioUnit getHandle();

protected:
  AudioUnit mUnit;
};

class MacDevice
{
public:
    MacDevice(int devId);
    ~MacDevice();

    bool open();
    void close();
    void setRender(bool render);
    void setCapture(bool capture);
    int getId();
    Format getFormat();

    DataConnection* connection();
    void setConnection(DataConnection* c);
    void provideAudioToSpeaker(int channels, void* buffer, int length);
    void obtainAudioFromMic(int channels, const void* buffer, int length);

protected:
    AudioDeviceID mDeviceId;
    bool mCapture, mRender;
    bool mActive;
    int mUsageCount;
    Mutex mGuard;

    CoreAudioUnit mAudioUnit;
    AudioComponent mComponent;
    AudioStreamBasicDescription mCaptureInputFormat, mCaptureOutputFormat, mRenderInputFormat, mRenderOutputFormat, mStreamFormat;
    AudioBufferList* mInputBufferList;
    DataConnection* mConnection;
    SpeexResampler mCaptureResampler, mRenderResampler;
    ByteBuffer mTail;
    DataWindow mInputBuffer, mOutputBuffer;
    bool createUnit(bool voice);
    void destroyUnit();
    void startStream();
    void stopStream();
    void setupStreamFormat();
    bool createResampleUnit(AudioStreamBasicDescription format);

    static OSStatus outputCallback( void                       *inRefCon,
                                    AudioUnitRenderActionFlags *ioActionFlags,
                                    const AudioTimeStamp       *inTimeStamp,
                                    UInt32                      inBusNumber,
                                    UInt32                      inNumberFrames,
                                    AudioBufferList            *ioData );

    static OSStatus inputCallback(void                       *inRefCon,
                                   AudioUnitRenderActionFlags *ioActionFlags,
                                   const AudioTimeStamp       *inTimeStamp,
                                   UInt32                      inBusNumber,
                                   UInt32                      inNumberFrames,
                                   AudioBufferList            *ioData);
#ifdef TARGET_IOS
    static void propListener(void *inClientData,
                                 AudioSessionPropertyID	inID,
                                 UInt32                 inDataSize,
                                 const void *           inData);
    static void interruptionListener(void *inClientData, UInt32 inInterruption);
#endif

};

typedef std::shared_ptr<MacDevice> PMacDevice;

class MacInputDevice: public InputDevice
{
public:
    MacInputDevice(int devId);
    ~MacInputDevice();

    bool open();
    void close();
    Format getFormat();

    bool fakeMode();
    void setFakeMode(bool fakemode);
    int readBuffer(void* buffer);
protected:
    PMacDevice mDevice;

};

class MacOutputDevice: public OutputDevice
{
public:
    MacOutputDevice(int devId);
    ~MacOutputDevice();

    bool open();
    void close();
    Format getFormat();

    bool fakeMode();
    void setFakeMode(bool fakemode);

protected:
    PMacDevice mDevice;
};

}

#endif // TARGET_OSX

#endif // __AUDIO_COREAUDIO_H
