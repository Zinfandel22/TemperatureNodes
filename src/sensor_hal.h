#pragma once
#include <stdint.h>

struct ShtReading
{
    float temperatureC;
    float humidityPct;
};

bool initSht40();
ShtReading readSht40();
float readBatteryVolts();