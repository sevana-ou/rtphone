#ifndef __HELPER_TIME_H
#define __HELPER_TIME_H

#include <time.h>

// Return current monotonic number of milliseconds starting from some point
extern double now_ms();

// Compare the timespec.
// Returns -1 if lhs < rhs, 1 if lhs > rhs, 0 if equal
extern int compare_timespec(const timespec& lhs, const timespec& rhs);
extern bool operator < (const timespec& lhs, const timespec& rhs);
extern bool operator == (const timespec& lhs, const timespec& rhs);
extern bool operator > (const timespec& lhs, const timespec& rhs);
extern bool is_zero(const timespec& ts);

#endif
