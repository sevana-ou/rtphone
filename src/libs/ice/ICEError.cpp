/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICEError.h"

using namespace ice;

std::string ErrorInfo::errorMsg(int errorCode)
{
  switch (errorCode)
  {
  case 403:  return "(Forbidden): The request was valid, but cannot be performed due \
      to administrative or similar restrictions.";

  case 437:  return  "(Allocation Mismatch): A request was received by the server that \
      requires an allocation to be in place, but there is none, or a \
      request was received which requires no allocation, but there is \
      one.";

  case 441:  return "(Wrong Credentials): The credentials in the (non-Allocate) \
      request, though otherwise acceptable to the server, do not match \
      those used to create the allocation.";

  case 442:  return "(Unsupported Transport Protocol): The Allocate request asked the \
      server to use a transport protocol between the server and the peer \
      that the server does not support.  NOTE: This does NOT refer to \
      the transport protocol used in the 5-tuple.";

  case 486:  return "(Allocation Quota Reached): No more allocations using this \
      username can be created at the present time.";

  case 508:  return "(Insufficient Capacity): The server is unable to carry out the \
      request due to some capacity limit being reached.  In an Allocate \
      response, this could be due to the server having no more relayed \
      transport addresses available right now, or having none with the \
      requested properties, or the one that corresponds to the specified \
      reservation token is not available.";
  }

  return "Unknown error.";

}

Exception::Exception(int errorCode, std::string errorMsg)
:mErrorCode(errorCode), mSubcode(0), mErrorMsg(errorMsg)
{
}

Exception::Exception(int code)
  :mErrorCode(code), mSubcode(0)
{
}

Exception::Exception(int code, int subcode)
  :mErrorCode(code), mSubcode(subcode)
{

}

Exception::Exception(const Exception& src)
:mErrorCode(src.mErrorCode), mErrorMsg(src.mErrorMsg)
{
}

Exception::~Exception()
{
}

int Exception::code()
{
  return mErrorCode;
}

int Exception::subcode()
{
  return mSubcode;
}

std::string Exception::message()
{
  return mErrorMsg;
}
