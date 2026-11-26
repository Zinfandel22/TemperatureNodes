#include <cassert>
#include <cmath>
#include <limits>
#include "Arduino.h"
#include "Wire.h"
#include "sensor_hal.h"

TestSerial Serial;
TestWire Wire;
bool sensorBeginSucceeds = false;
bool sensorReadSucceeds = false;
float sensorTemperature = 0;
float sensorHumidity = 50;

void analogReadResolution(int) {}
void analogSetAttenuation(int) {}
uint32_t analogReadMilliVolts(int) { return 0; }
void delayMicroseconds(unsigned) {}

int main() {
    assert(!initSht40());
    ShtReading reading = readSht40();
    assert(reading.temperatureF == -999.0f && reading.humidityPct == -999.0f);

    sensorBeginSucceeds = true;
    assert(initSht40());
    reading = readSht40();
    assert(reading.temperatureF == -999.0f && reading.humidityPct == -999.0f);

    sensorReadSucceeds = true;
    reading = readSht40();
    assert(reading.temperatureF == 32.0f && reading.humidityPct == 50.0f);

    sensorTemperature = 32.0f;
    reading = readSht40();
    assert(std::fabs(reading.temperatureF - 89.6f) < 0.001f);

    sensorTemperature = std::numeric_limits<float>::quiet_NaN();
    reading = readSht40();
    assert(reading.temperatureF == -999.0f && reading.humidityPct == 50.0f);
}
