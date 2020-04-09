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

class ThreadPool
{
public:
    ThreadPool(size_t, const std::string&);
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    ~ThreadPool();
    void setPause(bool v) { this->pause = v;}
private:
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
    // the task queue
    std::queue< std::function<void()> > tasks;
    
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop, pause;
    std::string name;
};
 
// the constructor just launches some amount of workers
inline ThreadPool::ThreadPool(size_t threads, const std::string& name)
    :   stop(false), pause(false), name(name)
{
    for(size_t i = 0;i<threads;++i)
        workers.emplace_back(
            [this, i]
            {
                ThreadHelper::setName(this->name + std::to_string(i));
                for(;;)
                {
                    std::function<void()> task; bool task_assigned = false;

                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex);
                        this->condition.wait(lock,
                            [this]{ return this->stop || !this->pause || !this->tasks.empty(); });
                        if (this->tasks.empty())
                            std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        if(this->stop && this->tasks.empty())
                            return;
                        if(this->pause)
                            continue;
                        if (this->tasks.size())
                        {
                            task = std::move(this->tasks.front()); task_assigned = true;
                            this->tasks.pop();
                        }
                    }
                    if (task_assigned)
                        task();
                }
            }
        );
}

// add new work item to the pool
template<class F, class... Args>
auto ThreadPool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type>
{
    using return_type = typename std::result_of<F(Args...)>::type;

    auto task = std::make_shared< std::packaged_task<return_type()> >(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);

        // don't allow enqueueing after stopping the pool
        if(stop)
            throw std::runtime_error("enqueue on stopped ThreadPool");

        tasks.emplace([task](){ (*task)(); });
    }
    condition.notify_one();
    return res;
}

// the destructor joins all threads
inline ThreadPool::~ThreadPool()
{
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop = true;
    }
    condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

#endif
