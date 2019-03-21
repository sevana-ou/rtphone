#ifndef __HL_SINGLETONE_H
#define __HL_SINGLETONE_H

#include <atomic>
#include <mutex>

template <class T>
class SafeSingleton
{
protected:
    static std::atomic<T*> SharedInstance;
    static std::mutex mMutex;
public:
    static T& instance()
    {
        T* tmp = SharedInstance.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            tmp = SharedInstance.load(std::memory_order_relaxed);
            if (tmp == nullptr)
            {
                tmp = new T();
                std::atomic_thread_fence(std::memory_order_release);
                SharedInstance.store(tmp, std::memory_order_relaxed);
            }
        }
        return *tmp;
    }

    template<typename X>
    static T& precreate(X n)
    {
        T* tmp = SharedInstance.load(std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_acquire);
        if (tmp == nullptr)
        {
            std::lock_guard<std::mutex> lock(mMutex);
            tmp = SharedInstance.load(std::memory_order_relaxed);
            if (tmp == nullptr)
            {
                tmp = new T(n);
                std::atomic_thread_fence(std::memory_order_release);
                SharedInstance.store(tmp, std::memory_order_relaxed);
                return *tmp;
            }
        }
        throw std::runtime_error("Singletone instance is created already");
    }
};

template <class T>
std::atomic<T*> SafeSingleton<T>::SharedInstance;
template <class T>
std::mutex SafeSingleton<T>::mMutex;

#endif
