/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifdef TARGET_OSX

#include "Audio_CoreAudio.h"
#include "../Helper/HL_Log.h"
//#include <qdebug.h>
#include <AudioUnit/AudioUnit.h>
#include <CoreAudio/CoreAudio.h>

using namespace Audio;

#define LOG_SUBSYSTEM "CoreAudio"

enum
{
    Bus_Speaker = 0,
    Bus_Microphone = 1
};

static inline short Float32ToInt16(Float32 v)
{
    //assert (v <= 1.0);
    int r = int(v * 32768);
    if (r >= 32768)
        return (short)32767;
    else
    if (r < -32768)
        return (short)-32768;
    else
        return (short)r;
}

static inline Float32 Int16ToFloat32(short v)
{
    Float32 r = Float32(v) / 32768.0;
    return r;
}

static inline Float32 StereoToMono(bool interleaved, Float32* buffer, int samples, int index, int channels)
{
    Float32 sum = 0;
    for (int i = 0; i<channels; i++)
    {
        if (!interleaved)
            sum += buffer[index * channels + i];
        else
            sum += buffer[samples * i + index];
    }
    return sum / channels;
}

static inline void MonoToStereo(bool interleaved, Float32 sample, Float32* buffer, int samples, int index, int channels)
{
    for (int i=0; i<channels; i++)
    {
        if (!interleaved)
            buffer[index * channels + i] = sample;
        else
            buffer[samples * i + index] = sample;
    }
}

static void propertyListenerCallback(void *inUserData, AudioQueueRef queueObject, AudioQueuePropertyID propertyID)
{
    // AudioPlayer *player = (AudioPlayer *) inUserData;
    // gets a reference to the playback object
    // [player.notificationDelegate updateUserInterfaceOnAudioQueueStateChange: player];
    // your notificationDelegate class implements the UI update method
}


CoreAudioUnit::CoreAudioUnit()
  :mUnit(0)
{
}

void CoreAudioUnit::open(bool voice)
{
  OSStatus osstatus = 0;
#ifdef TARGET_IOS
  UInt32 audioCategory = kAudioSessionCategory_PlayAndRecord;
  /* We want to be able to open playback and recording streams */
  ostatus = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                    sizeof(audioCategory),
                    &audioCategory);
  if (ostatus != kAudioSessionNoError)
  {
      ICELogError(<< "Cannot set audio session to PlaybackAndRecord category, error" << ostatus);
      throw AudioException(ERR_COREAUDIO, ostatus);
  }
#endif

  // Locate audio component
  mUnit = nullptr;
  AudioComponentDescription desc;
  desc.componentType = kAudioUnitType_Output;
#ifdef TARGET_IOS
  desc.componentSubType = voice ? kAudioUnitSubType_VoiceProcessingIO : kAudioUnitSubType_RemoteIO;
#else
  desc.componentSubType = kAudioUnitSubType_HALOutput;//voice ? kAudioUnitSubType_VoiceProcessingIO : kAudioUnitSubType_DefaultOutput;
#endif
  desc.componentManufacturer = kAudioUnitManufacturer_Apple;
  desc.componentFlags = 0;
  desc.componentFlagsMask = 0;

  AudioComponent component = AudioComponentFindNext(nullptr, &desc);
  if (component == nullptr)
  {
      ICELogError(<< "Cannot find audio component (null is returned)");
      throw AudioException(ERR_COREAUDIO, osstatus);
  }

  // Create audio unit
  osstatus = AudioComponentInstanceNew(component, &mUnit);
  if (osstatus != noErr)
  {
      ICELogError(<< "Cannot create audio component, error " << int(osstatus));
      throw AudioException(ERR_COREAUDIO, osstatus);
  }
}

void CoreAudioUnit::close()
{
  if (mUnit)
  {
    AudioUnitUninitialize(mUnit);
    AudioComponentInstanceDispose(mUnit);
    mUnit = nullptr;
  }
}

CoreAudioUnit::~CoreAudioUnit()
{
  close();
}

AudioStreamBasicDescription CoreAudioUnit::getFormat(int scope, int bus)
{
  AudioStreamBasicDescription result;
  UInt32 size = sizeof result;
  OSStatus ostatus = AudioUnitGetProperty(mUnit, kAudioUnitProperty_StreamFormat, scope, bus, &result, &size);
  if (ostatus != noErr)
  {
    ICELogError(<< "Cannot obtain stream format, error " << int(ostatus));
    throw AudioException(ERR_COREAUDIO, ostatus);
  }
  return result;
}

void CoreAudioUnit::setFormat(AudioStreamBasicDescription& format, int scope, int bus)
{
  OSStatus ostatus = AudioUnitSetProperty(mUnit,
               kAudioUnitProperty_StreamFormat,
               scope,
               bus,
               &format,
               sizeof(format));
  if (ostatus != noErr)
  {
    ICELogError(<< "Cannot set stream format, error " << int(ostatus));
    throw AudioException(ERR_COREAUDIO, ostatus);
  }
}

bool CoreAudioUnit::getEnabled(int scope, int bus)
{
  UInt32 wasEnabled = 0;
  UInt32 sizeWe = sizeof(wasEnabled);
  OSStatus ostatus = AudioUnitGetProperty(mUnit, kAudioOutputUnitProperty_EnableIO, scope, bus, &wasEnabled, &sizeWe);
  if (ostatus != noErr)
  {
      ICELogError(<< "Failed to get if input is already enabled on audio device");
  }
  return wasEnabled != 0;
}

void CoreAudioUnit::setEnabled(bool enabled, int scope, int bus)
{
  UInt32 enable = enabled ? 1 : 0;
  OSStatus ostatus = AudioUnitSetProperty(mUnit,
              kAudioOutputUnitProperty_EnableIO,
              scope,
              bus,
              &enable,
              sizeof(enable));

  if (ostatus != noErr)
  {
      ICELogError(<< "Cannot enable input on audio device , error " << int(ostatus));
      //throw AudioException(ERR_COREAUDIO, ostatus);
  }
}

void CoreAudioUnit::makeCurrent(AudioDeviceID deviceId, int scope, int bus)
{
  OSStatus ostatus = AudioUnitSetProperty(mUnit,
                     kAudioOutputUnitProperty_CurrentDevice,
                     scope,
                     bus,
                     &deviceId,
                     sizeof(deviceId));
  if (ostatus != noErr)
  {
      ICELogError(<< "Cannot make device " << int(deviceId) << " current, error " << ostatus);
      throw AudioException(ERR_COREAUDIO, ostatus);
  }
}


void CoreAudioUnit::setCallback(AURenderCallbackStruct cb, int callbackType, int scope, int bus)
{
  OSStatus ostatus = AudioUnitSetProperty(mUnit,
                     callbackType,
                     scope,
                     bus,
                     &cb,
                     sizeof(cb));
  if (ostatus != noErr)
  {
      ICELogError(<< "Cannot set callback pointer, error " << int(ostatus));
      throw AudioException(ERR_COREAUDIO, ostatus);
  }
}

void CoreAudioUnit::setBufferFrameSizeInMilliseconds(int ms)
{
#ifdef TARGET_IOS
  Float32 preferredBufferSize = Float32(ms) / 1000; // in seconds
  OSStatus ostatus = AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, sizeof(preferredBufferSize), &preferredBufferSize);
  if (ostatus != noErr)
  {
    ICELogError(<< "Cannot set audio buffer length to " << ms << " milliseconds");
  }
#endif
#ifdef TARGET_OSX
  // TODO: kAudioDevicePropertyBufferFrameSizeRange
#endif
}

int CoreAudioUnit::getBufferFrameSize()
{
  UInt32 bufsize = 0;
  UInt32 size = sizeof(UInt32);
  OSStatus ostatus = AudioUnitGetProperty(mUnit,
                             kAudioDevicePropertyBufferFrameSize,
                             kAudioUnitScope_Global,
                             Bus_Speaker,
                             &bufsize,
                             &size);
  if (ostatus != noErr)
  {
      ICELogError(<< "Cannot obtain input buffer size , error " << int(ostatus));
      throw AudioException(ERR_COREAUDIO, ostatus);
  }
  return int(bufsize);
}

void CoreAudioUnit::initialize()
{
  OSStatus ostatus = AudioUnitInitialize(mUnit);
  if (ostatus != noErr)
  {
    ICELogError(<< "Cannot initialize AudioUnit, error " << int(ostatus));
    throw AudioException(ERR_COREAUDIO, ostatus);
  }
}

AudioUnit CoreAudioUnit::getHandle()
{
  return mUnit;
}

OSStatus MacDevice::outputCallback( void                       *inRefCon,
                                AudioUnitRenderActionFlags *ioActionFlags,
                                const AudioTimeStamp       *inTimeStamp,
                                UInt32                      inBusNumber,
                                UInt32                      inNumberFrames,
                                AudioBufferList            *ioData )
{
  MacDevice* d = reinterpret_cast<MacDevice*>(inRefCon);
  if (!d)
      return noErr;

  if (!ioData->mNumberBuffers)
      return noErr;

  // Here deinterleaving is expected; it means coreaudio will request multiple buffers - one buffer for one channel
  // So only single buffer is filled from MacDevice; other are copies
  AudioBuffer& ab = ioData->mBuffers[0];
  if (ab.mNumberChannels == 1)
  {
    ICELogMedia(<< "CoreAudio output callback for mono " << (int)ab.mDataByteSize <<
                " bytes");
    memset(ab.mData, 0, ab.mDataByteSize);
    d->provideAudioToSpeaker(ab.mNumberChannels, ab.mData, ab.mDataByteSize);

    for (int i=1; i<ioData->mNumberBuffers; i++)
        memcpy(ioData->mBuffers[i].mData, ab.mData, ab.mDataByteSize);
  }
  else
  {
    ICELogMedia(<< "CoreAudio output callback for stereo " << (int)ab.mDataByteSize <<
                " bytes")
    // Iterate requested buffers
    for (unsigned i=0; i<ioData->mNumberBuffers; i++)
    {
      unsigned channels = ioData->mBuffers[i].mNumberChannels;
      short* dataPtr = (short*)ioData->mBuffers[i].mData;
      unsigned dataSize = ioData->mBuffers[i].mDataByteSize;

      // Preset with silence
      memset(dataPtr, 0, dataSize);

      // Find how much PCM16 samples are required to fill the requested space
      d->provideAudioToSpeaker(channels, dataPtr, dataSize);
    }
  }

  return noErr;
}

static char GlobalInputBuffer[AUDIO_MIC_BUFFER_COUNT * AUDIO_MIC_BUFFER_SIZE];

OSStatus MacDevice::inputCallback(void                       *inRefCon,
                               AudioUnitRenderActionFlags *ioActionFlags,
                               const AudioTimeStamp       *inTimeStamp,
                               UInt32                      inBusNumber,
                               UInt32                      inNumberFrames,
                               AudioBufferList            *ioData)
{
    //ICELogDebug(<< "CoreAudio input callback");
    MacDevice* d = reinterpret_cast<MacDevice*>(inRefCon);
    if (!d)
        return 0;

    OSStatus ostatus;
    AudioBuffer& b = d->mInputBufferList->mBuffers[0];
    //b.mDataByteSize = 65536;

    b.mNumberChannels = d->mStreamFormat.mChannelsPerFrame;
    b.mData = NULL;
    b.mDataByteSize = inNumberFrames * d->mStreamFormat.mChannelsPerFrame;

    // Render the unit to get input data
    ostatus = AudioUnitRender(d->mAudioUnit.getHandle(),
                  ioActionFlags,
                  inTimeStamp,
                  inBusNumber,
                  inNumberFrames,
                  d->mInputBufferList);

    if (ostatus != noErr)
    {
      ICELogError(<< "Cannot render input audio data, error " << int(ostatus));
    }
    else
    {
      d->obtainAudioFromMic(b.mNumberChannels, b.mData, b.mDataByteSize);
    }
    return noErr;
}

#ifdef TARGET_IOS
void MacDevice::propListener(void *inClientData,
                             AudioSessionPropertyID	inID,
                             UInt32                 inDataSize,
                             const void *           inData)
{
    MacDevice* d = reinterpret_cast<MacDevice*>(inClientData);
    CFDictionaryRef routeDictionary;
    CFNumberRef reason;
    SInt32 reasonVal;

    if (!d || inID != kAudioSessionProperty_AudioRouteChange)
        return;

    routeDictionary = (CFDictionaryRef)inData;
    reason = (CFNumberRef)CFDictionaryGetValue(routeDictionary, CFSTR(kAudioSession_AudioRouteChangeKey_Reason));
    CFNumberGetValue(reason, kCFNumberSInt32Type, &reasonVal);

    if (reasonVal != kAudioSessionRouteChangeReason_OldDeviceUnavailable)
    {
        return;
    }

    // Audio route changed. Nothing to do in this implementation.

}

void MacDevice::interruptionListener(void *inClientData, UInt32 inInterruption)
{
    MacDevice* d = reinterpret_cast<MacDevice*>(inClientData);

    if (inInterruption == kAudioSessionEndInterruption)
    {
        UInt32 audioCategory;
        OSStatus ostatus;

        /* Make sure that your application can receive remote control
         * events by adding the code:
         *     [[UIApplication sharedApplication]
         *      beginReceivingRemoteControlEvents];
         * Otherwise audio unit will fail to restart while your
         * application is in the background mode.
         */
        /* Make sure we set the correct audio category before restarting */
        audioCategory = kAudioSessionCategory_PlayAndRecord;
        ostatus = AudioSessionSetProperty(kAudioSessionProperty_AudioCategory,
                          sizeof(audioCategory),
                          &audioCategory);
        if (ostatus != kAudioSessionNoError)
        {
            ICELogError(<<"Cannot set the audio session category, error " << ostatus);
        }

        // Start stream
        d->startStream();
    }
    else
    if (inInterruption == kAudioSessionBeginInterruption)
    {
        d->stopStream();
    }
}

#endif
MacDevice::MacDevice(int devId)
:mDeviceId(devId), mCapture(false), mRender(false), mActive(false),
  mConnection(nullptr), mUsageCount(0)
{
  mInputBuffer.setCapacity(AUDIO_MIC_BUFFER_COUNT * AUDIO_MIC_BUFFER_SIZE);
  mOutputBuffer.setCapacity(AUDIO_SPK_BUFFER_SIZE);
}

MacDevice::~MacDevice()
{
}

DataConnection* MacDevice::connection()
{
  return mConnection;
}

void MacDevice::setConnection(DataConnection *c)
{
  mConnection = c;
}

void MacDevice::provideAudioToSpeaker(int channels, void *buffer, int length)
{
  if (!mConnection)
    return;

  mConnection->onSpkData(getFormat(), buffer, length);
  return;
}

void MacDevice::obtainAudioFromMic(int channels, const void *buffer, int length)
{
  if (!mConnection)
    return;

  // Put audio into internal buffer and see if there are ready AUDIO_BUFFER_SIZE chunks
  mConnection->onMicData(getFormat(), buffer, length);
  return;
}

bool MacDevice::open()
{
  Lock l(mGuard);
  mUsageCount++;
  if (mUsageCount  == 1)
  {
    if (!createUnit(true))
    {
      ICELogError(<< "Unable to create&adjust AudioUnit");
      return false;
      // TODO - enable stub for iOS
    }
    startStream();
  }
  return true;
}

void MacDevice::close()
{
  Lock l(mGuard);
  if (mUsageCount == 1)
  {
    stopStream();
    destroyUnit();
  }
  if (mUsageCount > 0)
    mUsageCount--;
}

void MacDevice::setRender(bool render)
{
  mRender = render;
}

void MacDevice::setCapture(bool capture)
{
  mCapture = capture;
}

int MacDevice::getId()
{
  return this->mDeviceId;
}

Format MacDevice::getFormat()
{
  Format result;
  result.mChannels = mStreamFormat.mChannelsPerFrame;
  result.mRate = mStreamFormat.mSampleRate;
  return result;
}

void MacDevice::setupStreamFormat()
{
  mStreamFormat.mSampleRate = AUDIO_SAMPLERATE;
  mStreamFormat.mFormatID = kAudioFormatLinearPCM;
  mStreamFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  mStreamFormat.mBitsPerChannel = AUDIO_SAMPLE_WIDTH;
  mStreamFormat.mChannelsPerFrame = AUDIO_CHANNELS;
  mStreamFormat.mBytesPerFrame = mStreamFormat.mChannelsPerFrame * mStreamFormat.mBitsPerChannel >> 3;
  mStreamFormat.mFramesPerPacket = 1;
  mStreamFormat.mBytesPerPacket = mStreamFormat.mFramesPerPacket * mStreamFormat.mBytesPerFrame;
}

bool MacDevice::createUnit(bool voice)
{
  OSStatus ostatus;

  mAudioUnit.open(voice);

  //if (mCapture != mAudioUnit.getEnabled(kAudioUnitScope_Input, Bus_Microphone))
    mAudioUnit.setEnabled(mCapture, kAudioUnitScope_Input, Bus_Microphone);
    //mAudioUnit.setEnabled(mCapture, kAudioUnitScope_Output, Bus_Microphone);

  //if (mRender != mAudioUnit.getEnabled(kAudioUnitScope_Output, Bus_Speaker))
    mAudioUnit.setEnabled(mRender, kAudioUnitScope_Output, Bus_Speaker);
    //mAudioUnit.setEnabled(mRender, kAudioUnitScope_Input, Bus_Speaker);


#ifdef TARGET_OSX
  // Open device

  if (mCapture)
    mAudioUnit.makeCurrent(mDeviceId, kAudioUnitScope_Global, Bus_Microphone);

  if (mRender)
    mAudioUnit.makeCurrent(mDeviceId, kAudioUnitScope_Global, Bus_Speaker);

#endif
  setupStreamFormat();

  if (mCapture)
  {
    mCaptureInputFormat = mAudioUnit.getFormat(kAudioUnitScope_Input, Bus_Microphone);
    mCaptureOutputFormat = mAudioUnit.getFormat(kAudioUnitScope_Output, Bus_Microphone);
    mStreamFormat.mSampleRate = mCaptureInputFormat.mSampleRate;
    if (mCaptureOutputFormat.mFormatFlags & kAudioFormatFlagIsNonInterleaved)
      ICELogDebug(<< "Input device produces noninterleaved data");

    mAudioUnit.setFormat(mStreamFormat, kAudioUnitScope_Output, Bus_Microphone);
    // Start resampler
    mCaptureResampler.start(mStreamFormat.mChannelsPerFrame, mStreamFormat.mSampleRate, AUDIO_SAMPLERATE);
  }

  if (mRender)
  {
    // Init resample
    mRenderOutputFormat = mAudioUnit.getFormat(kAudioUnitScope_Output, Bus_Speaker);
    mRenderInputFormat = mAudioUnit.getFormat(kAudioUnitScope_Input, Bus_Speaker);
    mAudioUnit.setFormat(mStreamFormat, kAudioUnitScope_Input, Bus_Speaker);
    // Start resample
    mRenderResampler.start(mStreamFormat.mChannelsPerFrame, AUDIO_SAMPLERATE, mStreamFormat.mSampleRate);

    // Set current render format - it is format required by unit from application; scope is Input and bus is 0 (speaker)
    //mAudioUnit.setFormat(mRenderInputFormat, kAudioUnitScope_Input, Bus_Speaker);

    // Configure callback
    AURenderCallbackStruct cb;
    cb.inputProc = outputCallback;
    cb.inputProcRefCon = this;
    mAudioUnit.setCallback(cb, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, Bus_Speaker);
  }

  if (mCapture)
  {
    AURenderCallbackStruct cb;
    cb.inputProc = inputCallback;
    cb.inputProcRefCon = this;
    mAudioUnit.setCallback(cb, kAudioOutputUnitProperty_SetInputCallback, kAudioUnitScope_Global, Bus_Microphone); //!!!
#ifdef TARGET_OSX
    AudioBuffer *ab;
    UInt32 size, bufsize;

    mAudioUnit.setBufferFrameSizeInMilliseconds(COREAUDIO_BUFFER_TIME);

    // Get device's buffer frame size
    bufsize = mAudioUnit.getBufferFrameSize();
    mInputBufferList = (AudioBufferList*)malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer));
    if (!mInputBufferList)
    {
        ICELogError(<< "No memory for buffer list");
        return false;
    }
    mInputBufferList->mNumberBuffers = 1;
    ab = &mInputBufferList->mBuffers[0];
    ab->mNumberChannels = mStreamFormat.mChannelsPerFrame;
    ab->mDataByteSize = 0;//bufsize * ab->mNumberChannels * mCaptureOutputFormat.mBitsPerChannel / 8 * 8;
    ab->mData = NULL;//malloc(ab->mDataByteSize);
    if (!ab->mData)
    {
        //ICELogError(<< "No memory for capture buffer");
    }
#endif
#ifdef TARGET_IOS
    /* We will let AudioUnitRender() to allocate the buffer
     * for us later
     */
    mInputBufferList = (AudioBufferList*)malloc(sizeof(AudioBufferList) + sizeof(AudioBuffer));
    if (!mInputBufferList)
    {
        ICELogError(<< "No memory for buffer list");
        return false;
    }
    mInputBufferList->mNumberBuffers = 1;
    mInputBufferList->mBuffers[0].mNumberChannels = 2;
#endif
    }

    // Finally bring it online
    mAudioUnit.initialize();
    return true;
}

void MacDevice::destroyUnit()
{
  mAudioUnit.close();
  mCaptureResampler.stop();
  mRenderResampler.stop();
}

void MacDevice::startStream()
{
  if (mActive)
    return;

#ifdef TARGET_IOS
  if (iosVersion() > 4)
  {
      UInt32 sessionMode = kAudioSessionMode_VoiceChat;
      AudioSessionSetProperty(kAudioSessionProperty_Mode, sizeof(sessionMode), &sessionMode);
  }

    // Share audio chain
    //UInt32 allowMixing = YES;
    //AudioSessionSetProperty (kAudioSessionProperty_OverrideCategoryMixWithOthers, sizeof (allowMixing), &allowMixing);

    // Activate audio chain
    AudioSessionSetActive(true);
#endif

  OSStatus ostatus;
  if (mAudioUnit.getHandle())
  {
    ostatus = AudioOutputUnitStart(mAudioUnit.getHandle());
    if (ostatus != noErr)
    {
      ICELogError(<< "Failed to start audio unit, error " << int(ostatus));
      return;
    }
  }
  mActive = true;
}

void MacDevice::stopStream()
{
  if (!mActive || !mAudioUnit.getHandle())
        return;
  OSStatus ostatus;
  ostatus = AudioOutputUnitStop(mAudioUnit.getHandle());
  if (ostatus != noErr)
  {
    ICELogError(<< "Failed to stop audio unit, error " << int(ostatus));
  }

#ifdef TARGET_IOS
  AudioSessionSetActive(false);
#endif
  mActive = false;
}

bool MacDevice::createResampleUnit(AudioStreamBasicDescription format)
{
    return true;
}

class MacDeviceList
{
public:
  MacDeviceList();
  ~MacDeviceList();
  PMacDevice findDevice(int devId);

  static MacDeviceList& instance();

protected:
  Mutex mGuard;
  std::vector<PMacDevice> mDeviceList;
  static MacDeviceList* mInstance;
};

MacDeviceList* MacDeviceList::mInstance = NULL;

MacDeviceList::MacDeviceList()
{

}

MacDeviceList::~MacDeviceList()
{

}

MacDeviceList& MacDeviceList::instance()
{
  if (!mInstance)
    mInstance = new MacDeviceList();
  return *mInstance;
}

PMacDevice MacDeviceList::findDevice(int devId)
{
  Lock l(mGuard);
  for (unsigned i=0; i<mDeviceList.size(); i++)
  {
        PMacDevice& d = mDeviceList[i];
        if (d->getId() == devId)
            return d;
  }

  PMacDevice d(new MacDevice(devId));
  mDeviceList.push_back(d);
  return d;
}

// Share list of opened devices
MacInputDevice::MacInputDevice(int devId)
    :InputDevice()
{
  // Look for MacDevice
  mDevice = MacDeviceList::instance().findDevice(devId);
  mDevice->setCapture(true);
  mDevice->setConnection(mConnection);
}

MacInputDevice::~MacInputDevice()
{
  mDevice.reset();
}

bool MacInputDevice::open()
{
  mDevice->setConnection(mConnection);
  return mDevice->open();
}

void MacInputDevice::close()
{
  mDevice->close();
}

Format MacInputDevice::getFormat()
{
  return mDevice->getFormat();
}

MacOutputDevice::MacOutputDevice(int devId)
    :OutputDevice()
{
  // Look for MacDevice
  mDevice = MacDeviceList::instance().findDevice(devId);
  mDevice->setRender(true);
  mDevice->setConnection(mConnection);
}

MacOutputDevice::~MacOutputDevice()
{
  mDevice.reset();
}

bool MacOutputDevice::open()
{
  mDevice->setConnection(mConnection);
  return mDevice->open();
}

void MacOutputDevice::close()
{
  mDevice->close();
}

Format MacOutputDevice::getFormat()
{
  return mDevice->getFormat();
}

MacEnumerator::MacEnumerator()
    :mDefaultInput(0), mDefaultOutput(0)
{

}

MacEnumerator::~MacEnumerator()
{

}

void MacEnumerator::open(int direction)
{
  mDirection = direction;

#ifdef TARGET_OSX
    mDefaultInput = 0;
    mDefaultOutput = 0;
    mDeviceList.clear();

    AudioObjectPropertyAddress addr;
    OSStatus osstatus;
    UInt32 devSize, size;
    // Get audio list size
    addr.mSelector = kAudioHardwarePropertyDevices;
    addr.mScope = kAudioObjectPropertyScopeGlobal;
    addr.mElement = kAudioObjectPropertyElementMaster;
    osstatus = AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &addr,
                                              0, NULL, &devSize);
    if (osstatus != noErr)
    {
        devSize = 0;
        return;
    }

    UInt32 devCount = devSize / sizeof(AudioDeviceID);
    std::vector<AudioDeviceID> deviceIds;
    deviceIds.resize(devCount);

    // Get actual list
    osstatus = AudioObjectGetPropertyData(kAudioObjectSystemObject, &addr,
                     0, NULL, &devSize, (void *)&deviceIds.front());
    if (osstatus != noErr)
        return;
    for (unsigned i=0; i<deviceIds.size(); i++)
    {
        DeviceInfo di;
        di.mId = deviceIds[i];
        getInfo(di);
        switch (mDirection)
        {
        case myMicrophone:
          if (di.mInputCount)
            mDeviceList.push_back(di);
          break;

        case mySpeaker:
          if (di.mOutputCount)
            mDeviceList.push_back(di);
          break;
        }
    }

    AudioDeviceID devId = kAudioObjectUnknown;

    /* Find default audio input device */
    if (direction == myMicrophone)
    {
      addr.mSelector = kAudioHardwarePropertyDefaultInputDevice;
      addr.mScope = kAudioObjectPropertyScopeGlobal;
      addr.mElement = kAudioObjectPropertyElementMaster;
      size = sizeof(devId);

      osstatus = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                           &addr, 0, NULL, &size, (void *)&devId);

      if (osstatus == noErr)
      {
        std::vector<DeviceInfo>::iterator r = std::find_if(mDeviceList.begin(), mDeviceList.end(),
                                                              [devId] (const DeviceInfo& di) { return di.mId == devId;});
          if (r != mDeviceList.end())
              mDefaultInput = r - mDeviceList.begin();
      }
    }

    /* Find default audio output device */
    if (direction == mySpeaker)
    {
      addr.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
      osstatus = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                           &addr, 0, NULL,
                           &size, (void *)&devId);
      if (osstatus == noErr)
      {
        std::vector<DeviceInfo>::iterator r = std::find_if(mDeviceList.begin(), mDeviceList.end(),
                                                           [devId](const DeviceInfo& di) { return di.mId == devId;});
          if (r != mDeviceList.end())
              mDefaultOutput = r - mDeviceList.begin();
      }
    }

#endif
}

void MacEnumerator::getInfo(DeviceInfo &di)
{
    UInt32 size;
    OSStatus osstatus;

    // Name
    AudioObjectPropertyAddress addr;
    addr.mSelector = kAudioDevicePropertyDeviceName;
    addr.mScope = kAudioObjectPropertyScopeGlobal;
    addr.mElement = kAudioObjectPropertyElementMaster;
    char name[256];
    size = sizeof(name);
    AudioObjectGetPropertyData(di.mId, &addr, 0, NULL, &size, (void *)name);
    di.mName = name;

    // Get the number of input channels
    addr.mSelector = kAudioDevicePropertyStreamConfiguration;
    addr.mScope = kAudioDevicePropertyScopeInput;
    size = 0;
    osstatus = AudioObjectGetPropertyDataSize(di.mId, &addr, 0, NULL, &size);
    AudioBufferList* buf = NULL;
    if (osstatus == noErr && size > 0)
        buf = (AudioBufferList*)malloc(size);

    if (buf)
    {
        UInt32 idx;

        /* Get the input stream configuration */
        osstatus = AudioObjectGetPropertyData(di.mId, &addr, 0, NULL, &size, buf);
        if (osstatus == noErr)
        {
            /* Count the total number of input channels in
             * the stream
             */
            for (idx = 0; idx < buf->mNumberBuffers; idx++)
            {
                di.mInputCount += buf->mBuffers[idx].mNumberChannels;
            }
        }
        free(buf);
        buf = NULL;
    }

    // Get the number of output channels
    addr.mScope = kAudioDevicePropertyScopeOutput;
    size = 0;
    osstatus = AudioObjectGetPropertyDataSize(di.mId, &addr, 0, NULL, &size);
    if (osstatus == noErr && size > 0)
        buf = (AudioBufferList*)malloc(size);
    if (buf)
    {
        UInt32 idx;

        /* Get the output stream configuration */
        osstatus = AudioObjectGetPropertyData(di.mId, &addr, 0, NULL, &size, buf);
        if (osstatus == noErr)
        {
            /* Count the total number of output channels in
             * the stream
             */
            for (idx = 0; idx < buf->mNumberBuffers; idx++)
            {
                di.mOutputCount += buf->mBuffers[idx].mNumberChannels;
            }
        }
        free(buf); buf = NULL;
    }

    /* Get default sample rate */
    addr.mSelector = kAudioDevicePropertyNominalSampleRate;
    addr.mScope = kAudioObjectPropertyScopeGlobal;
    size = sizeof(Float64);
    Float64 sampleRate;
    osstatus = AudioObjectGetPropertyData (di.mId, &addr, 0, NULL, &size, &sampleRate);
    if (osstatus == noErr)
        di.mDefaultRate = int(sampleRate);

    /* Set device capabilities here */
    if (di.mOutputCount > 0)
    {
        addr.mSelector = kAudioDevicePropertyVolumeScalar;
        addr.mScope = kAudioDevicePropertyScopeOutput;
        osstatus = AudioObjectHasProperty(di.mId, &addr);
        if (osstatus == noErr)
        {
            di.mCanChangeOutputVolume = true;
        }
    }
}

void MacEnumerator::close()
{

}

int MacEnumerator::count()
{
    return mDeviceList.size();
}

std::tstring MacEnumerator::nameAt(int index)
{
    return mDeviceList[index].mName;
}

int MacEnumerator::idAt(int index)
{
  return mDeviceList[index].mId;
}

int MacEnumerator::indexOfDefaultDevice()
{
    if (mDirection == myMicrophone)
        return mDefaultInput;
    else
        return mDefaultOutput;
}

#endif
