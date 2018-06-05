/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __SMART_POINTER_H
#define __SMART_POINTER_H

#include <memory>
#define SharedPtr std::shared_ptr

#include "HL_Sync.h"

#include <map>

class UsageCounter
{
public:
  UsageCounter();
  ~UsageCounter();
  int obtain(int usageId);
  int release(int usageId);
  int usageCount();
  void clear();

protected:
  typedef std::map<int, int> UsageMap;
  UsageMap mUsage;
  Mutex mGuard;
};

#endif
