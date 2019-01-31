/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "Audio_WavFile.h"
#include "helper/HL_Exception.h"
#include "helper/HL_String.h"
#include "helper/HL_Log.h"
#include "../config.h"

#include <memory.h>

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
:mHandle(NULL), mRate(0)
{
  mDataOffset = 0;
}

WavFileReader::~WavFileReader()
{
}

#define THROW_READERROR     throw Exception(ERR_WAVFILE_FAILED);

std::string WavFileReader::readChunk()
{
  char name[5];
  if (fread(name, 1, 4, mHandle) != 4)
    THROW_READERROR;

  name[4] = 0;
  std::string result = name;
  unsigned size;
  if (fread(&size, 4, 1, mHandle) != 1)
    THROW_READERROR;

  if (result == "fact")
    fread(&mDataLength, 4, 1, mHandle);
  else
  if (result != "data")
    fseek(mHandle, size, SEEK_CUR);
  else
    mDataLength = size;
	
  return result;
}

bool WavFileReader::open(const std::tstring& filename)
{
  LOCK;
  try
  {
  #ifdef WIN32
    mHandle = _wfopen(filename.c_str(), L"rb");
  #else
    mHandle = fopen(StringHelper::makeUtf8(filename).c_str(), "rb");
  #endif
    if (NULL == mHandle)
      return false;
    
    // Read the .WAV header
    char riff[4];
    if (fread(riff, 4, 1, mHandle) < 1)
		  THROW_READERROR;
  	
    if (!(riff[0] == 'R' && riff[1] == 'I' && riff[2] == 'F' && riff[3] == 'F'))
		  THROW_READERROR;

    // Read the file size
    unsigned int filesize = 0;
    if (fread(&filesize, 4, 1, mHandle) < 1)
      THROW_READERROR;

    char wavefmt[9];
    if (fread(wavefmt, 8, 1, mHandle) < 1)
      THROW_READERROR;

    wavefmt[8] = 0;
    if (strcmp(wavefmt, "WAVEfmt ") != 0)
      THROW_READERROR;

    unsigned fmtSize = 0;
    if (fread(&fmtSize, 4, 1, mHandle) < 1)
		  THROW_READERROR;

    unsigned fmtStart = ftell(mHandle);

    unsigned short formattag  = 0;
    if (fread(&formattag, 2, 1, mHandle) < 1)
      THROW_READERROR;
    
    if (formattag != 1/*WAVE_FORMAT_PCM*/)
      THROW_READERROR;

    mChannels = 0;
    if (fread(&mChannels, 2, 1, mHandle) < 1)
		  THROW_READERROR;
    
    mRate = 0;
    if (fread(&mRate, 4, 1, mHandle) < 1)
      THROW_READERROR;

    unsigned int avgbytespersec = 0;
    if (fread(&avgbytespersec, 4, 1, mHandle) < 1)
      THROW_READERROR;
    
    unsigned short blockalign = 0;
    if (fread(&blockalign, 2, 1, mHandle) < 1)
      THROW_READERROR;
    
    mBits = 0;
    if (fread(&mBits, 2, 1, mHandle) < 1)
      THROW_READERROR;
    
    if (mBits !=8 && mBits != 16)
      THROW_READERROR;

    // Read the "chunk"
    fseek(mHandle, fmtStart + fmtSize, SEEK_SET);
    //unsigned pos = ftell(mHandle);
    mDataLength = 0;
    while (readChunk() != "data")
      ;
  	
    mFileName = filename;
    mDataOffset = ftell(mHandle);

    mResampler.start(AUDIO_CHANNELS, mRate, AUDIO_SAMPLERATE);
  }
  catch(...)
  {
    fclose(mHandle); mHandle = NULL;
  }
  return isOpened();
}

void WavFileReader::close()
{
  LOCK;

  if (NULL != mHandle)
    fclose(mHandle);
  mHandle = NULL;
}

int WavFileReader::rate() const
{
  return mRate;
}

unsigned WavFileReader::read(void* buffer, unsigned bytes)
{
  return read((short*)buffer, bytes / (AUDIO_CHANNELS * 2)) * AUDIO_CHANNELS * 2;
}

unsigned WavFileReader::read(short* buffer, unsigned samples)
{
  LOCK;

  if (!mHandle)
    return 0;

  // Get number of samples that must be read from source file
  int requiredBytes = mResampler.getSourceLength(samples) * mChannels * mBits / 8;
  void* temp = alloca(requiredBytes);
  memset(temp, 0, requiredBytes);

  // Find required size of input buffer
  if (mDataLength)
  {
    unsigned filePosition = ftell(mHandle);

    // Check how much data we can read
    unsigned fileAvailable = mDataLength + mDataOffset - filePosition;
    requiredBytes = (int)fileAvailable < requiredBytes ? (int)fileAvailable : requiredBytes;
  }

  /*int readSamples = */fread(temp, 1, requiredBytes, mHandle);// / mChannels / (mBits / 8);
  size_t processedBytes = 0;
  size_t result = mResampler.processBuffer(temp, requiredBytes, processedBytes,
                                           buffer, samples * 2 * AUDIO_CHANNELS);

  return result / 2 / AUDIO_CHANNELS;
}

bool WavFileReader::isOpened()
{
  LOCK;

  return (mHandle != 0);
}

void WavFileReader::rewind()
{
  LOCK;
 
  if (mHandle)
    fseek(mHandle, mDataOffset, SEEK_SET);
}

std::tstring WavFileReader::filename() const
{
  LOCK;

  return mFileName;
}

unsigned WavFileReader::size() const
{
  LOCK;

  return mDataLength;
}

// ------------------------- WavFileWriter -------------------------
#define LOG_SUBSYTEM "WavFileWriter"

#define BITS_PER_CHANNEL  16

WavFileWriter::WavFileWriter()
:mHandle(NULL), mLengthOffset(0), mRate(AUDIO_SAMPLERATE), mChannels(1)
{
}

WavFileWriter::~WavFileWriter()
{
  close();
}

void WavFileWriter::checkWriteResult(int result)
{
  if (result < 1) 
    throw Exception(ERR_WAVFILE_FAILED, errno);
}

bool WavFileWriter::open(const std::tstring& filename, int rate, int channels)
{
  LOCK;

  close();
	
  mRate = rate;
  mChannels = channels;

#ifdef WIN32
  mHandle = _wfopen(filename.c_str(), L"wb");
#else
  mHandle = fopen(StringHelper::makeUtf8(filename).c_str(), "wb");
#endif
  if (NULL == mHandle)
  {
    ICELogError(<< "Failed to create .wav file: filename = " << StringHelper::makeUtf8(filename) << " , error = " << errno);
    return false;
  }

  // Write the .WAV header
  const char* riff = "RIFF";
  checkWriteResult( fwrite(riff, 4, 1, mHandle) );
  
  // Write the file size
  unsigned int filesize = 0;
  checkWriteResult( fwrite(&filesize, 4, 1, mHandle) );

  const char* wavefmt = "WAVEfmt ";
  checkWriteResult( fwrite(wavefmt, 8, 1, mHandle) );

  // Set the format description
  DWORD dwFmtSize = 16; /*= 16L*/;
  checkWriteResult( fwrite(&dwFmtSize, sizeof(dwFmtSize), 1, mHandle) );

  WaveFormatEx format;
  format.wFormatTag = WAVE_FORMAT_PCM; 
  checkWriteResult( fwrite(&format.wFormatTag, sizeof(format.wFormatTag), 1, mHandle) );
  
  format.nChannels = mChannels;
  checkWriteResult( fwrite(&format.nChannels, sizeof(format.nChannels), 1, mHandle) );

  format.nSamplesPerSec = mRate; 
  checkWriteResult( fwrite(&format.nSamplesPerSec, sizeof(format.nSamplesPerSec), 1, mHandle) );
  
  format.nAvgBytesPerSec = mRate * 2 * mChannels;
  checkWriteResult( fwrite(&format.nAvgBytesPerSec, sizeof(format.nAvgBytesPerSec), 1, mHandle) );

  format.nBlockAlign = 2 * mChannels;
  checkWriteResult( fwrite(&format.nBlockAlign, sizeof(format.nBlockAlign), 1, mHandle) );
  
  format.wBitsPerSample = BITS_PER_CHANNEL; 
  checkWriteResult( fwrite(&format.wBitsPerSample, sizeof(format.wBitsPerSample), 1, mHandle) );
  
  const char* data = "data";
  checkWriteResult( fwrite(data, 4, 1, mHandle));

  mFileName = filename;
  mWritten  = 0;

  mLengthOffset = ftell(mHandle);
  checkWriteResult( fwrite(&mWritten, 4, 1, mHandle) );

  return isOpened();
}

void WavFileWriter::close()
{
  LOCK;

  if (mHandle)
  {
    fclose(mHandle);
    mHandle = NULL;
  }
}

unsigned WavFileWriter::write(const void* buffer, unsigned bytes)
{
  LOCK;

  if (!mHandle)
    return 0;

  // Seek the end of file
  fseek(mHandle, 0, SEEK_END);
  mWritten += bytes;

  // Write the data
  fwrite(buffer, bytes, 1, mHandle);

  // Write file length
  fseek(mHandle, 4, SEEK_SET);
  unsigned int fl = mWritten + 36;
  fwrite(&fl, sizeof(fl), 1, mHandle);
  
  // Write data length
  fseek(mHandle, mLengthOffset, SEEK_SET);
  checkWriteResult( fwrite(&mWritten, 4, 1, mHandle) );

  return bytes;
}

bool WavFileWriter::isOpened()
{
  LOCK;

  return (mHandle != 0);
}

std::tstring WavFileWriter::filename()
{
  LOCK;

  return mFileName;
}

