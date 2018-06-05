/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_Pointer.h"

UsageCounter::UsageCounter()
{}

UsageCounter::~UsageCounter()
{}

int UsageCounter::obtain(int usageId)
{
  Lock l(mGuard);
  UsageMap::iterator usageIter = mUsage.find(usageId);
  if (usageIter != mUsage.end())
    usageIter->second = usageIter->second + 1;
  else
    mUsage[usageId] = 1;
  
  return usageCount();
}

int UsageCounter::release(int usageId)
{
  Lock l(mGuard);
  UsageMap::iterator usageIter = mUsage.find(usageId);
  if (usageIter == mUsage.end())
    return usageCount();

  usageIter->second = usageIter->second - 1;
  if (!usageIter->second)
    mUsage.erase(usageIter);
  
  return usageCount();
}

int UsageCounter::usageCount()
{
  Lock l(mGuard);
  UsageMap::const_iterator usageIter;
  int result = 0;
  for (usageIter = mUsage.begin(); usageIter != mUsage.end(); usageIter++)
    result += usageIter->second;

  return result;
}

void UsageCounter::clear()
{
  Lock l(mGuard);
  mUsage.clear();
}
