/* Copyright(C) 2007-2018 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */


#ifndef __AUDIO_WAVFILE_H
#define __AUDIO_WAVFILE_H

#include "helper/HL_Sync.h"
#include "helper/HL_Types.h"
#include "Audio_Resampler.h"

#include <stdio.h>
#include <string>
#include <memory>
#include <mutex>


namespace Audio
{

  class WavFileReader
  {
  protected:
    FILE* mHandle;
    short mChannels;
    short mBits;
    int mRate;
    std::tstring mFileName;
    mutable std::mutex mFileMtx;
    unsigned mDataOffset;
    unsigned mDataLength;
    Resampler mResampler;
    std::string readChunk();
  public:
    WavFileReader();
    ~WavFileReader();

    bool open(const std::tstring& filename);
    void close();
    bool isOpened();
    void rewind();
    int rate() const;

    // This method returns number of read bytes
    unsigned read(void* buffer, unsigned bytes);

    // This method returns number of read samples
    unsigned read(short* buffer, unsigned samples);
    std::tstring filename() const;
    unsigned size() const;
  };

  typedef std::shared_ptr<WavFileReader> PWavFileReader;

  class WavFileWriter
  {
  protected:
    FILE*           mHandle;        /// Handle of audio file.
    std::tstring    mFileName;      /// Path to requested audio file.
    std::mutex      mFileMtx;       /// Mutex to protect this instance.
    int             mWritten;       /// Amount of written data (in bytes)
    int             mLengthOffset;  /// Position of length field.
    int             mRate, mChannels;

    void checkWriteResult(int result);

  public:
    WavFileWriter();
    ~WavFileWriter();

    bool open(const std::tstring& filename, int rate, int channels);
    void close();
    bool isOpened();
    unsigned write(const void* buffer, unsigned bytes);
    std::tstring filename();
  };

  typedef std::shared_ptr<WavFileWriter> PWavFileWriter;

}

#endif
