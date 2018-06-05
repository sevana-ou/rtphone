/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_INTERFACE_H
#define __AUDIO_INTERFACE_H

#include <string>
#include "../config.h"
#include "../helper/HL_Types.h"
#include "../helper/HL_VariantMap.h"
#include "../helper/HL_Pointer.h"
#include "Audio_WavFile.h"
#include "Audio_Quality.h"

namespace Audio
{
  enum 
  {
    myMicrophone = 1,
    mySpeaker = 2
  };

  struct Format
  {
    int mRate;
    int mChannels;

    Format()
      :mRate(AUDIO_SAMPLERATE), mChannels(AUDIO_CHANNELS)
    {}

    Format(int rate, int channels)
            :mRate(rate), mChannels(channels)
    {}

    int samplesFromSize(int length) const
    {
      return length / 2 / mChannels;
    }

    // Returns milliseconds
    float timeFromSize(int length) const
    {
      return float(samplesFromSize(length) / (mRate / 1000.0));
    }

    float sizeFromTime(int milliseconds) const
    {
      return float((milliseconds * mRate) / 500.0 * mChannels);
    }

    std::string toString()
    {
      char buffer[64];
      sprintf(buffer, "%dHz %dch", mRate, mChannels);
      return std::string(buffer);
    }
  };

  class DataConnection
  {
  public:
    virtual void onMicData(const Format& format, const void* buffer, int length) = 0;
    virtual void onSpkData(const Format& format, void* buffer, int length) = 0;
  };


  class Device
  {
  public:
    Device();
    virtual ~Device();

    void setConnection(DataConnection* connection);
    DataConnection* connection();

    virtual bool open() = 0;
    virtual void close() = 0;
    virtual Format getFormat() = 0;
  protected:
    DataConnection* mConnection;
  };
  

  class InputDevice: public Device
  {
  public:
    InputDevice();
    virtual ~InputDevice();
    
    static InputDevice* make(int devId);
  };
  typedef std::shared_ptr<InputDevice> PInputDevice;

  class OutputDevice: public Device
  {
  public:
    OutputDevice();
    virtual ~OutputDevice();

    static OutputDevice* make(int devId);
  };
  typedef std::shared_ptr<OutputDevice> POutputDevice;

  class Enumerator
  {
  public:
    Enumerator();
    virtual ~Enumerator();
    int nameToIndex(const std::tstring& name);

    virtual void open(int direction) = 0;
    virtual void close() = 0;
    
    virtual int count() = 0;
    virtual std::tstring nameAt(int index) = 0;
    virtual int idAt(int index) = 0;
    virtual int indexOfDefaultDevice() = 0;
    
    static Enumerator* make(bool useNull = false);
  };

  class OsEngine
  {
  public:
    virtual void open() = 0;
    virtual void close() = 0;

    static OsEngine* instance();
  };
};

#endif
