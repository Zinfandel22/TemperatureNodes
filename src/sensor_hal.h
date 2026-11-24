#pragma once
#include <stdint.h>

struct ShtReading
{
    float temperatureF;
    float humidityPct;
};

bool initSht40();
ShtReading readSht40();
float readBatteryVolts();