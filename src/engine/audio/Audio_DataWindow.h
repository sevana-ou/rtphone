/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_BUFFER_H
#define __AUDIO_BUFFER_H

#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Sync.h"

namespace Audio
{
  class DataWindow
  {
  public:
    DataWindow();
    ~DataWindow();

    void setCapacity(int capacity);
    int capacity() const;

    void addZero(int length);
    void add(const void* data, int length);
    void add(short sample);
    int read(void* buffer, int length);
    void erase(int length = -1);
    const char* data() const;
    char* mutableData();
    int filled() const;
    void setFilled(int filled);
    void clear();

    short shortAt(int index) const;
    void setShortAt(short value, int index);
    void zero(int length);

    static void makeStereoFromMono(DataWindow& dst, DataWindow& src);

  protected:
    mutable Mutex mMutex;
    char* mData;
    int mFilled;
    int mCapacity;
  };
}
#endif
