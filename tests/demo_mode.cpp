#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>
#include "hub/DemoMode.h"

struct TestClient {
    float values[35] = {};
    std::string text[35];
    int writes[35] = {};
    int total = 0;

    void virtualWrite(int pin, float value) {
        values[pin] = value;
        writes[pin]++;
        total++;
    }
    void virtualWrite(int pin, int value) { virtualWrite(pin, static_cast<float>(value)); }
    void virtualWrite(int pin, const char *value) {
        text[pin] = value;
        writes[pin]++;
        total++;
    }
};

int randomCalls = 0;
long fixedRandom(long minValue, long) {
    randomCalls++;
    return minValue;
}

long varyingRandom(long minValue, long maxValue) {
    return minValue + (++randomCalls % (maxValue - minValue));
}

int main() {
    DemoMode demo;
    TestClient client;
    demo.publishIfDue(client, 0, "ONLINE --:--", fixedRandom);
    assert(client.total == 0);

    demo.setEnabled(true);
    demo.publishIfDue(client, 100, "ONLINE 01:23 PM", fixedRandom);
    assert(client.total == 23 && randomCalls == 10);
    for (int i = 0; i < 4; i++) {
        assert(client.values[i] == 65.0f && client.values[i + 4] == 35.0f);
        assert(client.text[i + 10] == "ONLINE 01:23 PM");
        assert(std::fabs(client.values[i + 20] - 3.8f) < 0.001f);
        assert(client.values[i + 30] == 75.0f);
    }
    assert(client.values[8] == 65.0f && client.values[9] == 35.0f);
    assert(client.text[14] == "ONLINE 01:23 PM");

    demo.publishIfDue(client, 30099, "ONLINE --:--", fixedRandom);
    assert(client.total == 23);
    demo.publishIfDue(client, 30100, "ONLINE --:--", fixedRandom);
    assert(client.total == 46 && randomCalls == 20);
    for (int i = 0; i < 5; i++) {
        int temperaturePin = i < 4 ? i : 8;
        int humidityPin = i < 4 ? i + 4 : 9;
        assert(std::fabs(client.values[temperaturePin] - 65.1f) < 0.001f);
        assert(std::fabs(client.values[humidityPin] - 35.1f) < 0.001f);
    }

    demo.setEnabled(false);
    demo.publishIfDue(client, 60100, "ONLINE --:--", fixedRandom);
    assert(client.total == 46);
    demo.setEnabled(true);
    demo.publishIfDue(client, UINT32_MAX - 100, "ONLINE --:--", fixedRandom);
    assert(client.total == 69);
    demo.publishIfDue(client, 29898, "ONLINE --:--", fixedRandom);
    assert(client.total == 69);
    demo.publishIfDue(client, 29899, "ONLINE --:--", fixedRandom);
    assert(client.total == 92);

    demo.setEnabled(true);
    demo.publishIfDue(client, 29900, "ONLINE --:--", fixedRandom);
    assert(client.total == 115);

    DemoMode independent;
    TestClient independentClient;
    randomCalls = 0;
    independent.setEnabled(true);
    independent.publishIfDue(independentClient, 0, "ONLINE --:--", varyingRandom);
    assert(randomCalls == 10);
    assert(independentClient.values[0] != independentClient.values[1]);
    assert(independentClient.values[4] != independentClient.values[5]);
    for (int i = 0; i < 5; i++) {
        int temperaturePin = i < 4 ? i : 8;
        int humidityPin = i < 4 ? i + 4 : 9;
        assert(independentClient.values[temperaturePin] >= 65.0f);
        assert(independentClient.values[temperaturePin] <= 80.0f);
        assert(independentClient.values[humidityPin] >= 35.0f);
        assert(independentClient.values[humidityPin] <= 65.0f);
        assert(independentClient.writes[temperaturePin] == 1);
        assert(independentClient.writes[humidityPin] == 1);
    }
}
