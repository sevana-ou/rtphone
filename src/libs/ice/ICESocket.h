/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_SOCKET_H
#define __ICE_SOCKET_H

#include "ICEPlatform.h"
#include "ICETypes.h"
#include <string>

namespace ice {

struct ICESocket
{
  std::string     mHostIP;
  unsigned short  mPort;
  SOCKET          mHandle;
  unsigned int    mPriority;

  ICESocket()
      :mPort(0), mHandle(INVALID_SOCKET), mPriority(0)
  {}

  ~ICESocket()
  {}
};

};

#endif
