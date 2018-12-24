/* Copyright(C) 2007-2018 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef __HL_SYNC_H
#define __HL_SYNC_H

#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>
#include <queue>
#include <assert.h>

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
public:
    Semaphore(unsigned int count = 0);

    void notify();
    void wait();
    bool waitFor(int milliseconds);

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    unsigned int m_count;
};

class ThreadHelper
{
public:
  static void setName(const std::string& name);
  static uint64_t getCurrentId();
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
    Block pull(int milliseconds);

protected:
    std::mutex mMutex;
    std::condition_variable mSignal;
    std::vector<Block> mBlockList;
};


// Timer Queue
//
// Allows execution of handlers at a specified time in the future
// Guarantees:
//  - All handlers are executed ONCE, even if canceled (aborted parameter will
//be set to true)
//      - If TimerQueue is destroyed, it will cancel all handlers.
//  - Handlers are ALWAYS executed in the Timer Queue worker thread.
//  - Handlers execution order is NOT guaranteed
//
class TimerQueue {
public:
    TimerQueue();
    ~TimerQueue();

    //! Adds a new timer
    // \return
    //  Returns the ID of the new timer. You can use this ID to cancel the
    // timer
    uint64_t add(int64_t milliseconds, std::function<void(bool)> handler);

    //! Cancels the specified timer
    // \return
    //  1 if the timer was cancelled.
    //  0 if you were too late to cancel (or the timer ID was never valid to
    // start with)
    size_t cancel(uint64_t id);

    //! Cancels all timers
    // \return
    //  The number of timers cancelled
    size_t cancelAll();

private:
    using Clock = std::chrono::steady_clock;
    TimerQueue(const TimerQueue&) = delete;
    TimerQueue& operator=(const TimerQueue&) = delete;

    void run();
    std::pair<bool, Clock::time_point> calcWaitTime();
    void checkWork();
    Semaphore m_checkWork;
    std::thread m_th;
    bool m_finish = false;
    uint64_t m_idcounter = 0;

    struct WorkItem {
        Clock::time_point end;
        uint64_t id;  // id==0 means it was cancelled
        std::function<void(bool)> handler;
        bool operator > (const WorkItem& other) const;
    };

    std::mutex m_mtx;
    // Inheriting from priority_queue, so we can access the internal container
    class Queue : public std::priority_queue<WorkItem, std::vector<WorkItem>,
                                             std::greater<WorkItem>> {
    public:
        std::vector<WorkItem>& getContainer();
    } m_items;
};

#endif
