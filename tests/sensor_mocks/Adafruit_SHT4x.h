#pragma once

struct sensors_event_t {
    float temperature;
    float relative_humidity;
};

constexpr int SHT4X_MED_PRECISION = 1;
extern bool sensorBeginSucceeds;
extern bool sensorReadSucceeds;
extern float sensorTemperature;
extern float sensorHumidity;

struct Adafruit_SHT4x {
    bool begin() { return sensorBeginSucceeds; }
    void setPrecision(int) {}
    bool getEvent(sensors_event_t *humidity, sensors_event_t *temperature) {
        if (!sensorReadSucceeds) return false;
        humidity->relative_humidity = sensorHumidity;
        temperature->temperature = sensorTemperature;
        return true;
    }
};
