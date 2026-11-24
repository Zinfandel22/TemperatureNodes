#include "hub/LastContactTime.h"
#include <cassert>
#include <cstring>

int main() {
    char result[9];
    tm localTime = {};

    localTime.tm_hour = 0;
    localTime.tm_min = 5;
    formatContactTime(localTime, result, sizeof(result));
    assert(strcmp(result, "12:05 AM") == 0);

    localTime.tm_hour = 12;
    localTime.tm_min = 0;
    formatContactTime(localTime, result, sizeof(result));
    assert(strcmp(result, "12:00 PM") == 0);

    localTime.tm_hour = 13;
    localTime.tm_min = 7;
    formatContactTime(localTime, result, sizeof(result));
    assert(strcmp(result, "01:07 PM") == 0);

    localTime.tm_hour = 23;
    localTime.tm_min = 59;
    formatContactTime(localTime, result, sizeof(result));
    assert(strcmp(result, "11:59 PM") == 0);
    return 0;
}
