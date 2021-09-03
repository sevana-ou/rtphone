/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __AUDIO_ANDROID_OBOE_H
#define __AUDIO_ANDROID_OBOE_H

#ifdef TARGET_ANDROID

#include "Audio_Interface.h"
#include "Audio_Helper.h"
#include "Audio_Resampler.h"
#include "Audio_DataWindow.h"
#include "../helper/HL_Pointer.h"
#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Exception.h"
#include "../helper/HL_Statistics.h"

#include <memory>
#include <string>

#include "oboe/Oboe.h"

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

    class AndroidInputDevice: public InputDevice, public oboe::AudioStreamCallback
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

        oboe::DataCallbackResult
        onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);


    protected:
        bool mActive = false;
        oboe::AudioStream* mRecordingStream = nullptr;
        PResampler mResampler;
        DataWindow mDeviceRateCache, mSdkRateCache;
        int mDeviceRate; // Actual rate of opened recorder
        int mBufferSize; // Size of buffer used for recording (at native sample rate)
        DataWindow mRecorderBuffer;
        std::condition_variable mDataCondVar;
        int mRecorderBufferIndex;
        std::mutex mMutex;
    };

class AndroidOutputDevice: public OutputDevice, public oboe::AudioStreamCallback
    {
    public:
        AndroidOutputDevice(int devId);
        ~AndroidOutputDevice();

        bool open();
        void close();
        Format getFormat();

        bool fakeMode();
        void setFakeMode(bool fakemode);

        oboe::DataCallbackResult onAudioReady(oboe::AudioStream *audioStream, void *audioData, int32_t numFrames);
        void onErrorAfterClose(oboe::AudioStream *stream, oboe::Result result);

        protected:
        std::mutex mMutex;
        int mDeviceRate = 0;
        oboe::AudioStream* mPlayingStream = nullptr;
        DataWindow mPlayBuffer;
        int mBufferIndex = 0, mBufferSize = 0;
        bool mInShutdown = false;
        bool mActive = false;

        // Statistics
        float mRequestedFrames = 0.0, mStartTime = 0.0, mEndTime = 0.0;
    };
}

#endif // TARGET_ANDROID

#endif // __AUDIO_ANDROID_H
