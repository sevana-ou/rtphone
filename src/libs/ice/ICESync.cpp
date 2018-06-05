/* Copyright(C) 2007-2014 VoIP objects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ICESync.h"

using namespace ice;

Mutex::Mutex()
{
#ifdef _WIN32
  InitializeCriticalSection(&mSection);
#else
  pthread_mutexattr_init(&mAttr);
  pthread_mutexattr_settype(&mAttr, PTHREAD_MUTEX_RECURSIVE);
  pthread_mutex_init(&mMutex, &mAttr);
#endif
}

Mutex::~Mutex()
{
#ifdef _WIN32
  DeleteCriticalSection(&mSection);
#else
  pthread_mutex_destroy(&mMutex);
  pthread_mutexattr_destroy(&mAttr);
#endif
}

void Mutex::lock()
{
#ifdef _WIN32
  EnterCriticalSection(&mSection);
#else
  pthread_mutex_lock(&mMutex);
#endif
}

void Mutex::unlock()
{
#ifdef _WIN32
  LeaveCriticalSection(&mSection);
#else
  pthread_mutex_unlock(&mMutex);
#endif
}


Lock::Lock(Mutex &mutex)
:mMutex(mutex)
{
  mMutex.lock();
}

Lock::~Lock()
{
  mMutex.unlock();
}

#ifndef _WIN32
static pthread_mutex_t DoGuard = PTHREAD_MUTEX_INITIALIZER ;
#endif

long Atomic::increment(long *value)
{
#ifdef _WIN32
  return ::InterlockedIncrement((LONG*)value);
#else
  pthread_mutex_lock(&DoGuard);
  (void)*value++;
  long result = *value;
  pthread_mutex_unlock(&DoGuard);
  return result;
#endif
}

long Atomic::decrement(long *value)
{
#ifdef _WIN32
  return ::InterlockedDecrement((LONG*)value);
#else
  pthread_mutex_lock(&DoGuard);
  (void)*value--;
  long result = *value;
  pthread_mutex_unlock(&DoGuard);
  return result;
#endif
}


void* ThreadInfo::currentThread()
{
#ifdef _WIN32
	return (void*)::GetCurrentThreadId();
#else
	return (void*)(::pthread_self());
#endif
}
