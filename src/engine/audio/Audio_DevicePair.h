/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_DEVICEPAIR_H
#define __AUDIO_DEVICEPAIR_H

#include "Audio_Interface.h"
#include "Audio_Player.h"
#include "Audio_Resampler.h"
#include "Audio_DataWindow.h"

//#define DUMP_NATIVEOUTPUT
//#define DUMP_NATIVEINPUT

namespace Audio
{

  class DevicePair: protected DataConnection
  {
  public:
    class Delegate: public DataConnection
    {
    public:
      virtual void deviceChanged(DevicePair* dpair) = 0;
    };

    DevicePair(bool aec = true, bool agc = true);
    virtual ~DevicePair();

    void setAec(bool aec);
    bool aec();
    void setAgc(bool agc);
    bool agc();

    VariantMap* config();
    void setConfig(VariantMap* config);

    PInputDevice input();
    void setInput(PInputDevice input);

    POutputDevice output();
    void setOutput(POutputDevice output);

    bool start();
    void stop();

    void setDelegate(Delegate* dc);
    Delegate* delegate();

    Player& player();

  protected:
    VariantMap* mConfig;
    PInputDevice mInput;
    POutputDevice mOutput;
    Delegate* mDelegate;
    bool mAec;
    bool mAgc;
    AgcFilter mAgcFilter;
    AecFilter mAecFilter;
    Player mPlayer;
    UniversalResampler mMicResampler, mSpkResampler;
    DataWindow mInputBuffer, mOutputBuffer, mAecSpkBuffer, mInputResampingData, mOutputNativeData, mOutput10msBuffer;

#ifdef DUMP_NATIVEOUTPUT
    std::shared_ptr<WavFileWriter> mNativeOutputDump;
#endif
#ifdef DUMP_NATIVEINPUT
    std::shared_ptr<WavFileWriter> mNativeInputDump;
#endif
    void onMicData(const Format& f, const void* buffer, int length);
    void onSpkData(const Format& f, void* buffer, int length);
    void processMicData(const Format& f, void* buffer, int length);
  };

  typedef std::shared_ptr<DevicePair> PDevicePair;
}

#endif
