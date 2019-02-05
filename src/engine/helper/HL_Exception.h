/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_EXCEPTION_H
#define __HL_EXCEPTION_H

#include <exception>
#include <memory.h>
#include <stdio.h>
#include <string.h>

enum
{
  ERR_MEDIA_SOCKET_FAILED   = 1,   // Failed to create media socket
  ERR_CANNOT_FIND_SESSION   = 2,   // Cannot find session
  ERR_NO_CREDENTIALS        = 3,   // No credentials to configure instance
  ERR_BAD_VARIANT_TYPE    = 4,   // Bad variant type conversion
  ERR_RINSTANCE           = 5,
  ERR_SRTP                = 6,   // libsrtp error
  ERR_WEBRTC              = 7,   // webrtc error
  ERR_NOMEM               = 8,   // no more memory
  ERR_WMME_FAILED         = 9,   // WMME error
  ERR_QPC                 = 10,  // QueryPerformanceCounter failed
  ERR_BAD_PARAM           = 11,  // Bad parameter
  ERR_NET_FAILED          = 12,  // Call to OS network subsystem failed
  ERR_NOT_IMPLEMENTED     = 13,  // Not implemented in this build
  ERR_MIXER_OVERFLOW      = 14,  // No more available channels in audio mixer
  ERR_WAVFILE_FAILED      = 15,  // Error with .wav file
  ERR_DSOUND              = 16,  // DSound error
  ERR_COREAUDIO           = 17,  // CoreAudio error
  ERR_CREATEWINDOW        = 18,  // CreateWindow failed
  ERR_REGISTERNOTIFICATION = 19, // RegisterDeviceNotification failed
  ERR_PCAP                = 20,  // Smth bad with libpcap
  ERR_CACHE_FAILED        = 21,  // Failed to open cache directory
  ERR_FILENOTOPEN         = 22,  // Cannot open the file
  ERR_OPENSLES            = 23   // OpenSL ES failed. Subcode has actual error code.
};

class Exception: public std::exception
{
public:
  Exception(int code, int subcode = 0)
    :mCode(code), mSubcode(subcode)
  {
    sprintf(mMessage, "%d-%d", code, subcode);
  }

  Exception(int code, const char* message)
  {
    if (message)
        strncpy(mMessage, message, (sizeof mMessage) - 1 );
  }

  Exception(const Exception& src)
    :mCode(src.mCode), mSubcode(src.mSubcode)
  {
    memcpy(mMessage, src.mMessage, sizeof mMessage);
  }

  ~Exception()
  { }
  
  int code() const
  {
    return mCode;
  }

  int subcode() const
  {
    return mSubcode;
  }

  const char* what() const noexcept
  {
    return mMessage;
  }

protected:
  int mCode, mSubcode;
  char mMessage[256];
};

#endif
