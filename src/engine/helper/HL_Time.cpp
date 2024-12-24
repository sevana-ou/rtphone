#include "HL_Time.h"

#include <time.h>
#include <chrono>
/* return current time in milliseconds */
double now_ms(void)
{
#if defined(TARGET_WIN)
    auto tp = std::chrono::steady_clock::now();
    auto result = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    return result;
#else
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;
#endif
}

int compare_timespec(const timespec& lhs, const timespec& rhs)
{
    if (lhs.tv_sec < rhs.tv_sec)
        return -1;
    if (lhs.tv_sec > rhs.tv_sec)
        return 1;

    if (lhs.tv_nsec < rhs.tv_nsec)
        return -1;
    if (lhs.tv_nsec > rhs.tv_nsec)
        return 1;

    return 0;
}

bool operator < (const timespec& lhs, const timespec& rhs)
{
    return compare_timespec(lhs, rhs) < 0;
}

bool operator == (const timespec& lhs, const timespec& rhs)
{
    return compare_timespec(lhs, rhs) == 0;
}

bool operator > (const timespec& lhs, const timespec& rhs)
{
    return compare_timespec(lhs, rhs) > 0;
}

bool is_zero(const timespec& ts)
{
    return !ts.tv_sec && !ts.tv_nsec;
}

