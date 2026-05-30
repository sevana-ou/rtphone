/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __AUDIO_BUFFER_H
#define __AUDIO_BUFFER_H

#include "../helper/HL_ByteBuffer.h"
#include "../helper/HL_Sync.h"
#include "Audio_Interface.h"

namespace Audio
{
class DataWindow
{
public:
    DataWindow();
    ~DataWindow();

    void        setCapacity(size_t capacity);
    size_t      capacity() const;

    void        addZero(size_t length);
    void        add(const void* data, size_t length);
    void        add(short sample);
    size_t      read(void* buffer, size_t length);
    void        erase(size_t length);
    const char* data() const;
    char*       mutableData();
    size_t      filled() const;
    void        setFilled(size_t filled);
    void        clear();

    short       shortAt(size_t index) const;
    void        setShortAt(short value, size_t index);
    void        zero(size_t length);
    size_t      moveTo(DataWindow& dst, size_t size /* in bytes*/ );

    std::chrono::milliseconds getTimeLength(const Format& fmt) const;

    static void makeStereoFromMono(DataWindow& dst, DataWindow& src);

protected:
    mutable Mutex mMutex;
    char*   mData = nullptr;
    size_t  mFilled = 0;
    size_t  mCapacity = 0;
};
}
#endif
