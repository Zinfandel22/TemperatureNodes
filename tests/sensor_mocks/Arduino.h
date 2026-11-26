#pragma once
#include <cmath>
#include <cstdint>

using std::isnan;
constexpr int A0 = 1;
constexpr int ADC_11db = 3;

struct TestSerial {
    void println(const char *) {}
};
extern TestSerial Serial;
void analogReadResolution(int);
void analogSetAttenuation(int);
uint32_t analogReadMilliVolts(int);
void delayMicroseconds(unsigned);
