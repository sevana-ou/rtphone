/* Copyright(C) 2007-2018 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef __AUDIO_WAVFILE_H
#define __AUDIO_WAVFILE_H

#include "helper/HL_Types.h"
#include "Audio_Resampler.h"

#include <stdio.h>
#include <string>
#include <memory>
#include <mutex>
#include <filesystem>
#include <fstream>


namespace Audio
{

class WavFileReader
{
protected:
    uint16_t                        mChannels = 0;
    uint16_t                        mBits = 0;
    int                             mSamplerate = 0;
    std::filesystem::path           mPath;
    mutable std::recursive_mutex    mFileMtx;
    size_t                          mDataOffset = 0;
    size_t                          mDataLength = 0;
    Resampler                       mResampler;
    unsigned                        mLastError = 0;
    std::unique_ptr<std::ifstream>  mInput;

    std::string         readChunk();
    void                readBuffer(void* buffer, size_t sz);       // This raises an exception if sz bytes are not read
    size_t              tryReadBuffer(void* buffer, size_t sz);  // This doesn't raise an exception

public:
    WavFileReader();
    ~WavFileReader();

    bool open(const std::filesystem::path& p);
    void close();
    bool isOpened();
    void rewind();
    int  samplerate() const;
    int  channels() const;

    // This method returns number of read bytes
    size_t read(void* buffer, size_t bytes);
    size_t  readRaw(void* buffer, size_t bytes);

    // This method returns number of read samples
    size_t read(short* buffer, size_t samples);
    size_t readRaw(short* buffer, size_t samples);

    std::filesystem::path path() const;
    size_t size() const;

    unsigned lastError() const;
};

typedef std::shared_ptr<WavFileReader> PWavFileReader;

class WavFileWriter
{
protected:
    std::unique_ptr<std::ofstream>  mOutput;            /// Handle of audio file.
    std::filesystem::path           mPath;              /// Path to requested audio file.
    mutable std::recursive_mutex    mFileMtx;           /// Mutex to protect this instance.
    size_t                          mWritten = 0;       /// Amount of written data (in bytes)
    size_t                          mLengthOffset = 0;  /// Position of length field.
    int                             mSamplerate = 0,
                                    mChannels = 0;

    void checkWriteResult(int result);
    void writeBuffer(const void* buffer, size_t sz);
public:
    WavFileWriter();
    ~WavFileWriter();

    bool open(const std::filesystem::path& p, int samplerate, int channels);
    void close();
    bool isOpened() const;
    size_t write(const void* buffer, size_t bytes);
    std::filesystem::path path() const;
};

typedef std::shared_ptr<WavFileWriter> PWavFileWriter;

}

#endif
