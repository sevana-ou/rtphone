#include "HL_Time.h"

#include <time.h>

/* return current time in milliseconds */
double now_ms(void) {
    struct timespec res;
    clock_gettime(CLOCK_MONOTONIC, &res);
    return 1000.0 * res.tv_sec + (double) res.tv_nsec / 1e6;
}