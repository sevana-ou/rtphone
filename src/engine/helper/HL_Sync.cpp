/* Copyright(C) 2007-2017 VoIPobjects (voipobjects.com)
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "HL_Sync.h"
#include <assert.h>
#include <atomic>
#include <memory.h>
#include <iostream>

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

uint64_t ThreadHelper::getCurrentId()
{
#if defined(TARGET_WIN)
    return static_cast<uint64_t>(GetCurrentThreadId());
#endif

#if defined(TARGET_LINUX)||defined(TARGET_OSX)
    // RPi builds want this!
    return (uint64_t)(pthread_self());
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

TimeHelper::ExecutionTime::ExecutionTime()
{
    mStart = TimeHelper::getTimestamp();
}

uint64_t TimeHelper::ExecutionTime::getSpentTime() const
{
    return TimeHelper::getTimestamp() - mStart;
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

    PBlock b = std::make_shared<Block>();
    b->resize(bytes);
    memcpy(b->data(), data, bytes);
    mBlockList.push_back(b);
    mSignal.notify_one();
}

BufferQueue::PBlock BufferQueue::pull(int milliseconds)
{
    std::unique_lock<std::mutex> l(mMutex);
    std::cv_status status = mBlockList.empty() ? std::cv_status::timeout : std::cv_status::no_timeout;

    if (mBlockList.empty())
        status = mSignal.wait_for(l, std::chrono::milliseconds(milliseconds));

    PBlock r;
    if (status == std::cv_status::no_timeout && !mBlockList.empty())
    {
        r = mBlockList.front();
        mBlockList.pop_front();
    }

    return r;
}

// ----------------- Semaphore ---------------------
Semaphore::Semaphore(unsigned int count)
    : m_count(count)
{}

void Semaphore::notify()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_count++;
    m_cv.notify_one();
}

void Semaphore::wait()
{
    std::unique_lock<std::mutex> lock(m_mtx);
    m_cv.wait(lock, [this]() { return m_count > 0; });
    m_count--;
}

bool Semaphore::waitFor(int milliseconds) {
    std::unique_lock<std::mutex> lock(m_mtx);

    if (!m_cv.wait_for(lock, std::chrono::milliseconds(milliseconds), [this]() { return m_count > 0; }))
        return false;

    m_count--;
    return true;
}

// ------------------- TimerQueue -------------------
TimerQueue::TimerQueue()
{
    m_th = std::thread([this] { run(); });
}

TimerQueue::~TimerQueue()
{
    cancelAll();
    // Abusing the timer queue to trigger the shutdown.
    add(0, [this](bool) { m_finish = true; });
    m_th.join();
}

//! Adds a new timer
// \return
//  Returns the ID of the new timer. You can use this ID to cancel the
// timer
uint64_t TimerQueue::add(int64_t milliseconds, std::function<void(bool)> handler) {
    WorkItem item;
    item.end = Clock::now() + std::chrono::milliseconds(milliseconds);
    item.handler = std::move(handler);

    std::unique_lock<std::mutex> lk(m_mtx);
    uint64_t id = ++m_idcounter;
    item.id = id;
    m_items.push(std::move(item));
    lk.unlock();

    // Something changed, so wake up timer thread
    m_checkWork.notify();
    return id;
}

//! Cancels the specified timer
// \return
//  1 if the timer was cancelled.
//  0 if you were too late to cancel (or the timer ID was never valid to
// start with)
size_t TimerQueue::cancel(uint64_t id) {
    // Instead of removing the item from the container (thus breaking the
    // heap integrity), we set the item as having no handler, and put
    // that handler on a new item at the top for immediate execution
    // The timer thread will then ignore the original item, since it has no
    // handler.
    std::unique_lock<std::mutex> lk(m_mtx);
    for (auto&& item : m_items.getContainer()) {
        if (item.id == id && item.handler) {
            WorkItem newItem;
            // Zero time, so it stays at the top for immediate execution
            newItem.end = Clock::time_point();
            newItem.id = 0;  // Means it is a canceled item
            // Move the handler from item to newitem.
            // Also, we need to manually set the handler to nullptr, since
            // the standard does not guarantee moving an std::function will
            // empty it. Some STL implementation will empty it, others will
            // not.
            newItem.handler = std::move(item.handler);
            item.handler = nullptr;
            m_items.push(std::move(newItem));
            // std::cout << "Cancelled timer. " << std::endl;
            lk.unlock();
            // Something changed, so wake up timer thread
            m_checkWork.notify();
            return 1;
        }
    }
    return 0;
}

//! Cancels all timers
// \return
//  The number of timers cancelled
size_t TimerQueue::cancelAll() {
    // Setting all "end" to 0 (for immediate execution) is ok,
    // since it maintains the heap integrity
    std::unique_lock<std::mutex> lk(m_mtx);
    for (auto&& item : m_items.getContainer()) {
        if (item.id) {
            item.end = Clock::time_point();
            item.id = 0;
        }
    }
    auto ret = m_items.size();

    lk.unlock();
    m_checkWork.notify();
    return ret;
}

void TimerQueue::run()
{
    ThreadHelper::setName("TimerQueue");
    while (!m_finish)
    {
        auto end = calcWaitTime();
        if (end.first)
        {
            // Timers found, so wait until it expires (or something else
            // changes)
            int milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>
                    (end.second - std::chrono::steady_clock::now()).count();
            //std::cout << "Waiting m_checkWork for " << milliseconds * 1000 << "ms." << std::endl;
            m_checkWork.waitFor(milliseconds * 1000);
        } else {
            // No timers exist, so wait forever until something changes
            m_checkWork.wait();
        }

        // Check and execute as much work as possible, such as, all expired
        // timers
        checkWork();
    }

    // If we are shutting down, we should not have any items left,
    // since the shutdown cancels all items
    assert(m_items.size() == 0);
}

std::pair<bool, TimerQueue::Clock::time_point> TimerQueue::calcWaitTime()
{
    std::lock_guard<std::mutex> lk(m_mtx);
    while (m_items.size()) {
        if (m_items.top().handler) {
            // Item present, so return the new wait time
            return std::make_pair(true, m_items.top().end);
        } else {
            // Discard empty handlers (they were cancelled)
            m_items.pop();
        }
    }

    // No items found, so return no wait time (causes the thread to wait
    // indefinitely)
    return std::make_pair(false, Clock::time_point());
}

void TimerQueue::checkWork() {
    std::unique_lock<std::mutex> lk(m_mtx);
    while (m_items.size() && m_items.top().end <= Clock::now()) {
        WorkItem item(std::move(m_items.top()));
        m_items.pop();

        lk.unlock();
        if (item.handler)
            item.handler(item.id == 0);
        lk.lock();
    }
}

bool TimerQueue::WorkItem::operator > (const TimerQueue::WorkItem& other) const {
    return end > other.end;
}

std::vector<TimerQueue::WorkItem>& TimerQueue::Queue::getContainer() {
    return this->c;
}
