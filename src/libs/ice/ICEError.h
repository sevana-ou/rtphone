/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_ERROR_H
#define __ICE_ERROR_H

#include <string>

namespace ice
{

  class ErrorInfo
  {
  public:
    static std::string errorMsg(int errorCode);

  };

  enum
  {
    GETIFADDRS_FAILED = 1,
    WRONG_CANDIDATE_TYPE,
    UDP_SUPPORTED_ONLY,
    NOT_ENOUGH_DATA,
    CANNOT_FIND_INTERFACES,
    NO_PRIORITY_ATTRIBUTE,
    CANNOT_FIND_ATTRIBUTE,
    UNKNOWN_ATTRIBUTE,
    WRONG_IP_ADDRESS
  };

  class Exception
  {
  public:

    Exception(int code, std::string msg);
    Exception(int code);
    Exception(int code, int subcode);

    Exception(const Exception& src);
    ~Exception();

    int           code();
    int           subcode();
    std::string   message();

  protected:
    int           mErrorCode;
    int           mSubcode;
    std::string   mErrorMsg;
  };

}

#endif
