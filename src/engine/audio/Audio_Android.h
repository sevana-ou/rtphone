/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __AUDIO_ANDROID_H
#define __AUDIO_ANDROID_H

#ifdef TARGET_ANDROID

#include "Audio_Interface.h"
#include "Audio_Helper.h"
#include "Audio_Resampler.h"
#include "Audio_DataWindow.h"
#include "../helper/HL_Pointer.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Exception.h"
#include <memory>
#include <string>

#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>

namespace Audio
{

    class AndroidEnumerator: public Enumerator
    {
    public:
        AndroidEnumerator();
        ~AndroidEnumerator();

        void open(int direction);
        void close();

        int count();
        std::string nameAt(int index);
        int idAt(int index);
        int indexOfDefaultDevice();

    protected:
    };

    class AndroidInputDevice: public InputDevice
    {
    public:
        AndroidInputDevice(int devId);
        ~AndroidInputDevice();

        bool open();
        void close();
        Format getFormat();

        bool fakeMode();
        void setFakeMode(bool fakemode);
        int readBuffer(void* buffer);
        bool active() const;

    protected:
        bool mActive = false;
        SLObjectItf mRecorderObject = nullptr;
        SLRecordItf mRecorderInterface = nullptr;
        SLAndroidSimpleBufferQueueItf mRecorderBufferInterface = nullptr;
        SLAndroidConfigurationItf mAndroidCfg = nullptr;

        PResampler mResampler;
        DataWindow mDeviceRateCache, mSdkRateCache;
        int mDeviceRate; // Actual rate of opened recorder
        int mBufferSize; // Size of buffer used for recording (at native sample rate)
        DataWindow mRecorderBuffer;
        std::condition_variable mDataCondVar;
        int mRecorderBufferIndex;
        std::mutex mMutex;

        void internalOpen(int rateCode, int rate);
        void internalClose();
        void handleCallback(SLAndroidSimpleBufferQueueItf bq);
        static void DeviceCallback(SLAndroidSimpleBufferQueueItf bq, void* context);
    };

    class AndroidOutputDevice: public OutputDevice
    {
    public:
        AndroidOutputDevice(int devId);
        ~AndroidOutputDevice();

        bool open();
        void close();
        Format getFormat();

        bool fakeMode();
        void setFakeMode(bool fakemode);

    protected:
        std::mutex mMutex;
        int mDeviceRate = 0;
        SLObjectItf mMixer = nullptr;
        SLObjectItf mPlayer = nullptr;
        SLPlayItf mPlayerControl = nullptr;
        SLAndroidSimpleBufferQueueItf  mBufferQueue = nullptr;
        SLAndroidConfigurationItf mAndroidConfig = nullptr;
        SLEffectSendItf mEffect = nullptr;

        DataWindow mPlayBuffer;
        int mBufferIndex = 0, mBufferSize = 0;
        bool mInShutdown = false;

        void internalOpen(int rateId, int rate, bool voice);
        void internalClose();

        void handleCallback(SLAndroidSimpleBufferQueueItf bq);
        static void DeviceCallback(SLAndroidSimpleBufferQueueItf bq, void* context);

    };

    class OpenSLEngine: public OsEngine
    {
    public:
        OpenSLEngine();
        ~OpenSLEngine();

        // open() / close() methods are based on usage counting.
        // It means every close() call must be matched by corresponding open() call.
        // True audio engine close will happen only on last close() call.
        void open() override;
        void close() override;

        SLEngineItf getNativeEngine() const;

        static OpenSLEngine& instance();

    protected:
        std::mutex mMutex;
        int mUsageCounter = 0;
        SLObjectItf mEngineObject = nullptr;
        SLEngineItf mEngineInterface = nullptr;

        void internalOpen();
        void internalClose();
    };
}

#endif // TARGET_ANDROID

#endif // __AUDIO_ANDROID_H
