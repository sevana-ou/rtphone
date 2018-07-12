/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_Sync.h"
#include <assert.h>
#include <atomic>

#ifdef TARGET_OSX
# include <libkern/OSAtomic.h>
#endif

#ifdef TARGET_WIN
# include <Windows.h>
#endif

void SyncHelper::delay(unsigned int microseconds)
{
#ifdef TARGET_WIN
  ::Sleep(microseconds/1000);
#endif
#if defined(TARGET_OSX) || defined(TARGET_LINUX)
  timespec requested, remaining;
  requested.tv_sec = microseconds / 1000000;
  requested.tv_nsec = (microseconds % 1000000) * 1000;
  remaining.tv_nsec = 0;
  remaining.tv_sec = 0;
  nanosleep(&requested, &remaining);
#endif
}

long SyncHelper::increment(long *value)
{
  assert(value);
#ifdef TARGET_WIN
  return ::InterlockedIncrement((LONG*)value);
#elif TARGET_OSX
  return OSAtomicIncrement32((int32_t*)value);
#elif TARGET_LINUX
  return -1;
#endif
}

// ------------------- ThreadHelper -------------------
void ThreadHelper::setName(const std::string &name)
{
#if defined(TARGET_LINUX)
  pthread_setname_np(pthread_self(), name.c_str());
#endif
}

// ------------------- TimeHelper ---------------
using namespace std::chrono;

static uint64_t TimestampStartPoint = duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
static time_t TimestampBase = time(nullptr);

uint64_t TimeHelper::getTimestamp()
{
  time_point<steady_clock> t = steady_clock::now();

  uint64_t ms = duration_cast< milliseconds >(t.time_since_epoch()).count();

  return ms - TimestampStartPoint + TimestampBase * 1000;
}

uint64_t TimeHelper::getUptime()
{
  time_point<steady_clock> t = steady_clock::now();

  uint64_t ms = duration_cast< milliseconds >(t.time_since_epoch()).count();

  return ms - TimestampStartPoint;
}

uint32_t TimeHelper::getDelta(uint32_t later, uint32_t earlier)
{
  if (later > earlier)
    return later - earlier;

  if (later < earlier && later < 0x7FFFFFFF && earlier >= 0x7FFFFFFF)
    return 0xFFFFFFFF - earlier + later;

  return 0;
}

// --------------- BufferQueue -----------------
BufferQueue::BufferQueue()
{

}

BufferQueue::~BufferQueue()
{

}

void BufferQueue::push(const void* data, int bytes)
{
    std::unique_lock<std::mutex> l(mMutex);

    Block b = std::make_shared<std::vector<unsigned char>>();
    b->resize(bytes);
    memcpy(b->data(), data, bytes);
    mBlockList.push_back(b);
}

BufferQueue::Block BufferQueue::pull(std::chrono::milliseconds timeout)
{
    std::unique_lock<std::mutex> l(mMutex);
    std::cv_status status = std::cv_status::no_timeout;

    while (mBlockList.empty() && status == std::cv_status::no_timeout)
        status = mSignal.wait_for(l, timeout);

    Block r;
    if (status == std::cv_status::no_timeout)
        r = mBlockList.front(); mBlockList.erase(mBlockList.begin());

    return r;
}
