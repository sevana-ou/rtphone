/* Copyright(C) 2007-2026 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include "Audio_DataWindow.h"

using namespace Audio;

DataWindow::DataWindow()
{}

DataWindow::~DataWindow()
{
    if (mData)
    {
        free(mData);
        mData = nullptr;
    }
}

void DataWindow::setCapacity(size_t capacity)
{
    Lock l(mMutex);

    // The window only ever grows; a smaller request keeps the current buffer.
    if (capacity <= mCapacity)
        return;

    size_t tail  = capacity - mCapacity;
    char* buffer = mData;
    mData = (char*)realloc(mData, capacity);
    if (!mData)
    {
        // Realloc failed
        mData = buffer;
        throw std::bad_alloc();
    }
    if (tail > 0)
        memset(mData + mCapacity, 0, tail);
    mCapacity = capacity;
}

void DataWindow::addZero(size_t length)
{
    Lock l(mMutex);

    if (length > mCapacity)
        length = mCapacity;

    int avail = mCapacity - mFilled;

    if (avail < length)
    {
        memmove(mData, mData + length - avail, mFilled - (length - avail));
        mFilled -= length - avail;
    }
    memset(mData + mFilled, 0, length);
    mFilled += length;
}


void DataWindow::add(const void* data, size_t length)
{
    Lock l(mMutex);

    if (length > mCapacity)
    {
        // Use latest bytes from data buffer in this case.
        data = (char*)data + length - mCapacity;
        length = mCapacity;
    }

    // Check how much free space we have
    int avail = mCapacity - mFilled;

    if (avail < length)
    {
        // Find the portion of data to move & save
        int delta = length - avail;

        // Move the data
        if (mFilled - delta > 0)
            memmove(mData, mData + delta, mFilled - delta);
        mFilled -= delta;
    }

    memcpy(mData + mFilled, data, length);
    mFilled += length;
}

void DataWindow::add(short sample)
{
    add(&sample, sizeof sample);
}

void DataWindow::erase(size_t length)
{
    Lock l(mMutex);
    if (length > mFilled)
        length = mFilled;
    if (length != mFilled)
        memmove(mData, mData + length, mFilled - length);
    mFilled -= length;
}

const char* DataWindow::data() const
{
    return mData;
}

char* DataWindow::mutableData()
{
    return mData;
}

void DataWindow::clear()
{
    Lock l(mMutex);
    mFilled = 0;
}

short DataWindow::shortAt(size_t index) const
{
    Lock l(mMutex);
    assert(index < mFilled / 2);
    return ((short*)mData)[index];
}

void DataWindow::setShortAt(short value, size_t index)
{
    Lock l(mMutex);
    assert(index < mFilled / 2);
    ((short*)mData)[index] = value;
}

size_t DataWindow::read(void* buffer, size_t length)
{
    Lock l(mMutex);
    if (length > mFilled)
        length = mFilled;
    if (length)
    {
        if (buffer)
            memcpy(buffer, mData, length);
        if (length < mFilled)
            memmove(mData, mData+length, mFilled - length);
        mFilled -= length;
    }
    return length;
}

size_t DataWindow::filled() const
{
    Lock l(mMutex);
    return mFilled;
}

void DataWindow::setFilled(size_t filled)
{
    Lock l(mMutex);
    if (filled > mCapacity)
        throw std::bad_alloc();
    mFilled = filled;
}

size_t DataWindow::capacity() const
{
    Lock l(mMutex);
    return mCapacity;
}

void DataWindow::zero(size_t length)
{
    Lock l(mMutex);
    assert(length <= mCapacity);
    mFilled = length;
    memset(mData, 0, mFilled);
}

size_t DataWindow::moveTo(DataWindow& dst, size_t size)
{
    Lock l(mMutex);

    size_t avail = std::min(size, (size_t)filled());
    if (avail != 0)
    {
        dst.add(mData, avail);
        erase(avail);
    }
    return avail;
}

std::chrono::milliseconds DataWindow::getTimeLength(const Audio::Format& fmt) const
{
    Lock l(mMutex);
    return std::chrono::milliseconds(mFilled / sizeof(short) / fmt.channels() / (fmt.rate()/ 1000));
}

void DataWindow::makeStereoFromMono(DataWindow& dst, DataWindow& src)
{
    Lock lockDst(dst.mMutex), lockSrc(src.mMutex);

    dst.setCapacity(src.filled()*2);
    short* input = (short*)src.mutableData();
    short* output = (short*)dst.mutableData();

    for (int i=0; i<src.filled()/2; i++)
        output[i*2] = output[i*2+1] = input[i];
    dst.mFilled = src.filled() * 2;
}
