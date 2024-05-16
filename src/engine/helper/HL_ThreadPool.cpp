#include "HL_ThreadPool.h"

thread_pool::thread_pool(size_t num_of_threads, const std::string& name)
{
    this->name = name;
    if (!num_of_threads)
        num_of_threads = std::thread::hardware_concurrency();

    for(size_t idx = 0; idx < num_of_threads; idx++)
        this->workers.push_back(std::thread(&thread_pool::run_worker, this));
}

// Add new work item to the pool
void thread_pool::enqueue(const thread_pool::task& t)
{
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        this->tasks.push(t);
    }
    this->condition.notify_one();
}

void thread_pool::wait(std::chrono::milliseconds interval)
{
    while (size() != 0)
        std::this_thread::sleep_for(interval);
}

size_t thread_pool::size()
{
    std::unique_lock l(this->queue_mutex);
    return this->tasks.size();
}

// the destructor joins all threads
thread_pool::~thread_pool()
{
    {
        std::unique_lock<std::mutex> lock(this->queue_mutex);
        stop = true;
    }
    this->condition.notify_all();
    for(std::thread &worker: workers)
        worker.join();
}

void thread_pool::run_worker()
{
    ThreadHelper::setName(this->name);
    task t;
    while (!this->stop)
    {
        {
            std::unique_lock<std::mutex> lock(this->queue_mutex);

            this->condition.wait(lock, [this]{return !this->tasks.empty() || this->stop;});
            t = this->tasks.front();
            this->tasks.pop();
        }
        t(); // function<void()> type
    }
}
