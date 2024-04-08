/* Copyright(C) 2007-2018 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <assert.h>
#include "Audio_DataWindow.h"

using namespace Audio;

DataWindow::DataWindow()
{
    mFilled = 0;
    mData = NULL;
    mCapacity = 0;
}

DataWindow::~DataWindow()
{
    if (mData)
        free(mData);
}

void DataWindow::setCapacity(int capacity)
{
    Lock l(mMutex);
    int tail  = capacity - mCapacity;
    mData = (char*)realloc(mData, capacity);
    if (tail > 0)
        memset(mData + mCapacity, 0, tail);
    mCapacity = capacity;
}

void DataWindow::addZero(int length)
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


void DataWindow::add(const void* data, int length)
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

void DataWindow::erase(int length)
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

short DataWindow::shortAt(int index) const
{
    Lock l(mMutex);
    assert(index < mFilled / 2);
    return ((short*)mData)[index];
}

void DataWindow::setShortAt(short value, int index)
{
    Lock l(mMutex);
    assert(index < mFilled / 2);
    ((short*)mData)[index] = value;
}

int DataWindow::read(void* buffer, int length)
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

int DataWindow::filled() const
{
    Lock l(mMutex);
    return mFilled;
}

void DataWindow::setFilled(int filled)
{
    Lock l(mMutex);
    mFilled = filled;
}

int DataWindow::capacity() const
{
    Lock l(mMutex);
    return mCapacity;
}

void DataWindow::zero(int length)
{
    Lock l(mMutex);
    assert(length <= mCapacity);
    mFilled = length;
    memset(mData, 0, mFilled);
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
