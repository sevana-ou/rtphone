/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Audio_WavFile.h"
#include "helper/HL_Exception.h"
#include "helper/HL_String.h"
#include "helper/HL_Log.h"
#include "../engine_config.h"

#include <memory.h>
#include <assert.h>

#ifndef WORD
# define WORD unsigned short
#endif
#ifndef DWORD
# define DWORD unsigned int
#endif

typedef struct {
    WORD  wFormatTag;
    WORD  nChannels;
    DWORD nSamplesPerSec;
    DWORD nAvgBytesPerSec;
    WORD  nBlockAlign;
    WORD  wBitsPerSample;
    WORD  cbSize;
}
WaveFormatEx;

#define WAVE_FORMAT_PCM 1

#define LOG_SUBSYSTEM "WavFileReader"

#define LOCK std::unique_lock<std::recursive_mutex> lock(mFileMtx);

using namespace Audio;

// ---------------------- WavFileReader -------------------------
WavFileReader::WavFileReader()
    :mSamplerate(0), mLastError(0), mChannels(0), mBits(0), mDataLength(0)
{
    mDataOffset = 0;
}

WavFileReader::~WavFileReader()
{
}

#define THROW_READERROR     throw Exception(ERR_WAVFILE_FAILED);

std::string WavFileReader::readChunk()
{
    char name[5] = {0};
    readBuffer(name, 4);

    std::string result = name;
    uint32_t size = 0;
    readBuffer(&size, 4);

    if (result == "fact")
    {
        uint32_t dataLength = 0;
        readBuffer(&dataLength, sizeof dataLength);
        mDataLength = dataLength;
    }
    else
    if (result != "data")
        mInput->seekg(size, std::ios_base::beg);
    else
        mDataLength = size;

    return result;
}

void  WavFileReader::readBuffer(void* buffer, size_t sz)
{
    auto p = mInput->tellg();
    mInput->read(reinterpret_cast<char*>(buffer), sz);
    if (mInput->tellg() - p != sz)
        throw Exception(ERR_WAVFILE_FAILED);
}

size_t WavFileReader::tryReadBuffer(void* buffer, size_t sz)
{
    auto p = mInput->tellg();
    mInput->read(reinterpret_cast<char*>(buffer), sz);
    return mInput->tellg() - p;
}

bool WavFileReader::open(const std::filesystem::path& p)
{
    LOCK;
    try
    {
        mPath = p;
        mInput = std::make_unique<std::ifstream>(p, std::ios::binary | std::ios::in);
        if (!mInput->is_open())
        {
#if defined(TARGET_ANDROID) || defined(TARGET_LINUX) || defined(TARGET_OSX)
            mLastError = errno;
#endif
#if defined(TARGET_WIN)
            mLastError = GetLastError();
#endif
            return false;
        }
        mLastError = 0;

        // Read the .WAV header
        char riff[4];
        readBuffer(riff, sizeof riff);

        if (!(riff[0] == 'R' && riff[1] == 'I' && riff[2] == 'F' && riff[3] == 'F'))
            THROW_READERROR;

        // Read the file size
        uint32_t filesize = 0;
        readBuffer(&filesize, sizeof(filesize));

        char wavefmt[9] = {0};
        readBuffer(wavefmt, 8);
        if (strcmp(wavefmt, "WAVEfmt ") != 0)
            THROW_READERROR;

        uint32_t fmtSize = 0;
        readBuffer(&fmtSize, sizeof(fmtSize));

        auto fmtStart = mInput->tellg();

        uint16_t formattag  = 0;
        readBuffer(&formattag, sizeof(formattag));

        if (formattag != 1/*WAVE_FORMAT_PCM*/)
            THROW_READERROR;

        mChannels = 0;
        readBuffer(&mChannels, sizeof(mChannels));

        mSamplerate = 0;
        readBuffer(&mSamplerate, sizeof(mSamplerate));

        uint32_t avgbytespersec = 0;
        readBuffer(&avgbytespersec, sizeof(avgbytespersec));

        uint16_t blockalign = 0;
        readBuffer(&blockalign, sizeof(blockalign));

        mBits = 0;
        readBuffer(&mBits, sizeof(mBits));

        if (mBits !=8 && mBits != 16)
            THROW_READERROR;

        // Look for the chunk 'data'
        mInput->seekg(fmtStart + std::streampos(fmtSize));

        mDataLength = 0;
        while (readChunk() != "data")
            ;

        mDataOffset = mInput->tellg();
        mResampler.start(AUDIO_CHANNELS, mSamplerate, AUDIO_SAMPLERATE);
    }
    catch(...)
    {
        mInput.reset();
        mLastError = static_cast<unsigned>(-1);
    }
    return isOpened();
}

void WavFileReader::close()
{
    LOCK;
    mInput.reset();
}

int WavFileReader::samplerate() const
{
    return mSamplerate;
}

int WavFileReader::channels() const
{
    return mChannels;
}

size_t WavFileReader::read(void* buffer, size_t bytes)
{
    return read((short*)buffer, bytes / (AUDIO_CHANNELS * 2)) * AUDIO_CHANNELS * 2;
}

size_t WavFileReader::readRaw(void* buffer, size_t bytes)
{
    return readRaw((short*)buffer, bytes / channels() / sizeof(short)) * channels() * sizeof(short);
}

size_t WavFileReader::read(short* buffer, size_t samples)
{
    LOCK;

    if (!mInput)
        return 0;

    // Get number of samples that must be read from source file
    size_t requiredBytes = mResampler.getSourceLength(samples) * mChannels * mBits / 8;
    bool useHeap = requiredBytes > sizeof mTempBuffer;
    void* temp;
    if (useHeap)
        temp = malloc(requiredBytes);
    else
        temp = mTempBuffer;

    memset(temp, 0, requiredBytes);

    // Find required size of input buffer
    if (mDataLength)
    {
        auto filePosition = mInput->tellg();

        // Check how much data we can read
        size_t fileAvailable = mDataLength + mDataOffset - filePosition;
        requiredBytes = fileAvailable < requiredBytes ? fileAvailable : requiredBytes;
    }

    size_t readBytes = tryReadBuffer(temp, requiredBytes);

    size_t processedBytes = 0;
    size_t result = mResampler.processBuffer(temp, readBytes, processedBytes,
                                             buffer, samples * 2 * AUDIO_CHANNELS);

    if (useHeap)
        free(temp);
    return result / 2 / AUDIO_CHANNELS;
}


size_t WavFileReader::readRaw(short* buffer, size_t samples)
{
    LOCK;

    if (!mInput)
        return 0;

    // Get number of samples that must be read from source file
    size_t requiredBytes = samples * channels() * sizeof(short);

    // Find required size of input buffer
    if (mDataLength)
    {
        auto filePosition = mInput->tellg();

        // Check how much data we can read
        size_t fileAvailable = mDataLength + mDataOffset - filePosition;
        requiredBytes = (int)fileAvailable < requiredBytes ? (int)fileAvailable : requiredBytes;
    }

    size_t readBytes = tryReadBuffer(buffer, requiredBytes);
    return readBytes / channels() / sizeof(short);
}

bool WavFileReader::isOpened()
{
    LOCK;
    if (!mInput)
        return false;
    return mInput->is_open();
}

void WavFileReader::rewind()
{
    LOCK;
    if (mInput)
        mInput->seekg(mDataOffset);
}

std::filesystem::path WavFileReader::path() const
{
    LOCK;
    return mPath;
}

size_t WavFileReader::size() const
{
    LOCK;
    return mDataLength;
}

unsigned WavFileReader::lastError() const
{
    return mLastError;
}
// ------------------------- WavFileWriter -------------------------
#define LOG_SUBSYTEM "WavFileWriter"

#define BITS_PER_CHANNEL  16

WavFileWriter::WavFileWriter()
    :mLengthOffset(0), mSamplerate(AUDIO_SAMPLERATE), mChannels(1), mWritten(0)
{}

WavFileWriter::~WavFileWriter()
{
    close();
}

void WavFileWriter::checkWriteResult(int result)
{
    if (result < 1)
        throw Exception(ERR_WAVFILE_FAILED, errno);
}

void WavFileWriter::writeBuffer(const void* buffer, size_t sz)
{
    if (!mOutput)
        return;

    auto p = mOutput->tellp();
    mOutput->write(reinterpret_cast<const char*>(buffer), sz);
    if (mOutput->tellp() - p != sz)
        throw Exception(ERR_WAVFILE_FAILED);
}

bool WavFileWriter::open(const std::filesystem::path& p, int samplerate, int channels)
{
    LOCK;
    close();
    mSamplerate = samplerate;
    mChannels = channels;

    mOutput = std::make_unique<std::ofstream>(p, std::ios::binary | std::ios::trunc);
    if (!mOutput)
    {
        int errorcode = errno;
        ICELogError(<< "Failed to create .wav file: filename = " << p << " , error = " << errorcode);
        return false;
    }

    // Write the .WAV header
    const char* riff = "RIFF";
    writeBuffer(riff, 4);

    // Write the file size
    uint32_t filesize = 0;
    writeBuffer(&filesize, sizeof filesize);

    const char* wavefmt = "WAVEfmt ";
    writeBuffer(wavefmt, 8);

    // Set the format description
    uint32_t dwFmtSize = 16; /*= 16L*/;
    writeBuffer(&dwFmtSize, sizeof(dwFmtSize));

    WaveFormatEx format;
    format.wFormatTag = WAVE_FORMAT_PCM;
    writeBuffer(&format.wFormatTag, sizeof(format.wFormatTag));

    format.nChannels = mChannels;
    writeBuffer(&format.nChannels, sizeof(format.nChannels));

    format.nSamplesPerSec = mSamplerate;
    writeBuffer(&format.nSamplesPerSec, sizeof(format.nSamplesPerSec));

    format.nAvgBytesPerSec = mSamplerate * 2 * mChannels;
    writeBuffer(&format.nAvgBytesPerSec, sizeof(format.nAvgBytesPerSec));

    format.nBlockAlign = 2 * mChannels;
    writeBuffer(&format.nBlockAlign, sizeof(format.nBlockAlign));

    format.wBitsPerSample = BITS_PER_CHANNEL;
    writeBuffer(&format.wBitsPerSample, sizeof(format.wBitsPerSample));

    const char* data = "data";
    writeBuffer(data, 4);

    mPath = p;
    mWritten  = 0;

    mLengthOffset = mOutput->tellp();
    writeBuffer(&mWritten, sizeof mWritten);

    return isOpened();
}

void WavFileWriter::close()
{
    LOCK;
    mOutput.reset();
}

size_t WavFileWriter::write(const void* buffer, size_t bytes)
{
    LOCK;

    if (!mOutput)
        return 0;

    // Seek the end of file - here new data will be written
    mOutput->seekp(0, std::ios_base::end);
    mWritten += bytes;

    // Write the data
    writeBuffer(buffer, bytes);

    // Write file length
    mOutput->seekp(4, std::ios_base::beg);
    uint32_t fl = mWritten + 36;
    writeBuffer(&fl, sizeof(fl));

    // Write data length
    mOutput->seekp(mLengthOffset, std::ios_base::beg);
    writeBuffer(&mWritten, sizeof(mWritten));

    return bytes;
}

bool WavFileWriter::isOpened() const
{
    LOCK;
    return mOutput.get();
}

std::filesystem::path WavFileWriter::path() const
{
    LOCK;
    return mPath;
}

