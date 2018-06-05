/* Copyright(C) 2007-2017 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __ICE_SYNC_H
#define __ICE_SYNC_H

#ifdef _WIN32
# include <winsock2.h>
# include <windows.h>
#else
# include <pthread.h>
#endif

namespace ice
{


class Mutex
{
public:
  Mutex();
  ~Mutex();
  void lock();
  void unlock();

protected:
#ifdef _WIN32
  CRITICAL_SECTION mSection;
#else
  pthread_mutexattr_t mAttr;
  pthread_mutex_t mMutex;
#endif
};

class Lock
{
public:
  Lock(Mutex& mutex);
  ~Lock();

protected:
  Mutex& mMutex;
};

class Atomic
{
public:
  static long increment(long* value);
  static long decrement(long* value);
};

class ThreadInfo
{
public:
  static void* currentThread();
};

}

#endif
