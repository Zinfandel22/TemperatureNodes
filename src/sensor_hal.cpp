#include "sensor_hal.h"
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_SHT4x.h>

constexpr uint8_t BATTERY_ADC_PIN = A0; // GPIO 1
constexpr float   DIVIDER_RATIO   = 2.00f;

static Adafruit_SHT4x sht4;
static bool sht4Ready = false;

bool initSht40() {
    Wire.begin(); // XIAO ESP32S3 default: SDA = D4 (GPIO5), SCL = D5 (GPIO6)
    if (!sht4.begin()) {
        sht4Ready = false;
        return false;
    }
    sht4.setPrecision(SHT4X_MED_PRECISION);
    sht4Ready = true;
    return true;
}

ShtReading readSht40() {
    if (!sht4Ready) {
        return {-999.0f, -999.0f};
    }
    sensors_event_t humidity, temp;
    sht4.getEvent(&humidity, &temp);
    return {temp.temperature, humidity.relative_humidity};
}

float readBatteryVolts() {
    analogReadResolution(12);
    analogSetAttenuation(ADC_11db);

    (void)analogReadMilliVolts(BATTERY_ADC_PIN);
    uint32_t totalMilliVolts = 0;
    for (uint8_t i = 0; i < 8; i++) {
        totalMilliVolts += analogReadMilliVolts(BATTERY_ADC_PIN);
        delayMicroseconds(200);
    }
    return ((totalMilliVolts / 8.0f) / 1000.0f) * DIVIDER_RATIO;
}