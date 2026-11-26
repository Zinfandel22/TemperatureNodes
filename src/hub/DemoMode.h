#pragma once

#include <stdint.h>
#include "shared_protocol.h"

class DemoMode {
public:
    void setEnabled(bool enabled) {
        enabled_ = enabled;
        if (enabled) publishNow_ = true;
    }

    bool enabled() const { return enabled_; }

    bool due(uint32_t now) const {
        return enabled_ && (publishNow_ || uint32_t(now - lastPublishMs_) >= 30000);
    }

    template <typename Client>
    void publishIfDue(Client &client, uint32_t now, const char *contact,
                      long (*randomValue)(long, long)) {
        if (!due(now)) return;
        publishNow_ = false;
        lastPublishMs_ = now;

        for (uint8_t i = 0; i <= MAX_REMOTE_NODES; i++) {
            long temperature = randomValue(650, 801);
            long humidity = randomValue(350, 651);
            if (hasPrevious_ && temperature == temperatures_[i]) {
                temperature = temperature == 800 ? 650 : temperature + 1;
            }
            if (hasPrevious_ && humidity == humidities_[i]) {
                humidity = humidity == 650 ? 350 : humidity + 1;
            }
            temperatures_[i] = temperature;
            humidities_[i] = humidity;
            if (i < MAX_REMOTE_NODES) {
                client.virtualWrite(i, temperature / 10.0f);
                client.virtualWrite(i + 4, humidity / 10.0f);
                client.virtualWrite(i + 10, contact);
                client.virtualWrite(i + 20, 3.8f);
                client.virtualWrite(i + 30, 75);
            } else {
                client.virtualWrite(8, temperature / 10.0f);
                client.virtualWrite(9, humidity / 10.0f);
                client.virtualWrite(14, contact);
            }
        }
        hasPrevious_ = true;
    }

private:
    bool enabled_ = false;
    bool publishNow_ = false;
    bool hasPrevious_ = false;
    uint32_t lastPublishMs_ = 0;
    long temperatures_[MAX_REMOTE_NODES + 1] = {};
    long humidities_[MAX_REMOTE_NODES + 1] = {};
};
