#ifndef __HL_THREAD_POOL_H
#define __HL_THREAD_POOL_H

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>
#include <optional>
#include "HL_Sync.h"

class thread_pool
{
public:
    typedef std::function<void()> task;

    thread_pool(size_t num_of_threads, const std::string& thread_name);
    ~thread_pool();

    void enqueue(const task& task);
    void wait(std::chrono::milliseconds interval = std::chrono::milliseconds(50));
    size_t size();
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;

    // the task queue
    std::queue< task > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic_bool stop = false;

    // thread name prefix for worker threads
    std::string name;

    void run_worker();
};
#endif
