#pragma once

#include <stddef.h>
#include <stdio.h>
#include <time.h>

constexpr time_t MIN_VALID_CLOCK_TIME = 1609459200;

inline void formatContactTime(const tm &localTime, char *buffer, size_t size) {
    int hour = localTime.tm_hour % 12;
    if (hour == 0) hour = 12;
    snprintf(buffer, size, "%02d:%02d %s", hour, localTime.tm_min,
             localTime.tm_hour < 12 ? "AM" : "PM");
}
