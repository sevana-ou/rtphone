/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_SYNC_H
#define __HL_SYNC_H

#include <mutex>
#include <condition_variable>
#include <chrono>
#include <thread>
#include <vector>

typedef std::recursive_mutex Mutex;
typedef std::unique_lock<std::recursive_mutex> Lock;

class SyncHelper
{
public:
  static void delay(unsigned microseconds);
  static long increment(long* value);
};

class Semaphore
{
private:
    unsigned int m_uiCount;
    std::mutex m_mutex;
    std::condition_variable m_condition;

public:
    inline Semaphore(unsigned int uiCount)
       : m_uiCount(uiCount) { }

    inline void Wait()
    {
        std::unique_lock< std::mutex > lock(m_mutex);
        m_condition.wait(lock,[&]()->bool{ return m_uiCount>0; });
        --m_uiCount;
    }

    template< typename R,typename P >
    bool Wait(const std::chrono::duration<R,P>& crRelTime)
    {
        std::unique_lock< std::mutex > lock(m_mutex);
        if (!m_condition.wait_for(lock,crRelTime,[&]()->bool{ return m_uiCount>0; }))
            return false;
        --m_uiCount;
        return true;
    }

    inline void Signal()
    {
        std::unique_lock< std::mutex > lock(m_mutex);
        ++m_uiCount;
        m_condition.notify_one();
    }
};

class ThreadHelper
{
public:
  static void setName(const std::string& name);
};

class TimeHelper
{
public:
  // Returns current timestamp in milliseconds
  static uint64_t getTimestamp();

  // Returns uptime (of calling process) in milliseconds
  static uint64_t getUptime();

  // Finds time delta between 'later' and 'earlier' time points.
  // Handles cases when clock is wrapped.
  static uint32_t getDelta(uint32_t later, uint32_t earlier);

  class ExecutionTime
  {
  public:
      ExecutionTime();
      uint64_t getSpentTime() const;
  protected:
      uint64_t mStart;
  };
};

	
class BufferQueue
{
public:
    BufferQueue();
    ~BufferQueue();

    typedef std::shared_ptr<std::vector<unsigned char>> Block;

    void push(const void* data, int bytes);
    Block pull(std::chrono::milliseconds timeout);

protected:
    std::mutex mMutex;
    std::condition_variable mSignal;
    std::vector<Block> mBlockList;
};

#endif
