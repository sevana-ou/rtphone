/* Copyright(C) 2007-2016 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_IOS_SUPPORT_H
#define __ICE_IOS_SUPPORT_H

#include <vector>
#include "ICEAddress.h"

namespace ice
{
  int getIosIp(int networkType, int family, char* adress);
  int fillIosInterfaceList(int family, int networkType, std::vector<ice::NetworkAddress>& output);
}

#endif