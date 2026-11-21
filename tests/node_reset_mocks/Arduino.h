#pragma once
#include <stdint.h>

constexpr int D0 = 1;
constexpr int LOW = 0;
constexpr int HIGH = 1;
constexpr int INPUT = 0;
constexpr int INPUT_PULLUP = 2;
constexpr int ADC_11db = 3;

struct TestSerial {
    void println(const char *) {}
};

extern TestSerial Serial;
void pinMode(int pin, int mode);
void delay(uint32_t ms);
uint32_t millis();
int digitalRead(int pin);
void analogSetAttenuation(int attenuation);
uint32_t analogReadMilliVolts(int pin);
