#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include "hub/DemoMode.h"
#include "hub/LoggedBlynk.h"

struct TestOutput {
    std::ostringstream stream;

    template <typename Value>
    void print(const Value &value) { stream << value; }
    template <typename Value>
    void println(const Value &value) { stream << value << '\n'; }
    void println(uint8_t value) { stream << static_cast<int>(value) << '\n'; }
    void println(float value, int precision) {
        stream << std::fixed << std::setprecision(precision) << value << '\n';
    }
};

struct TestClient {
    int writes = 0;
    int lastPin = -1;
    float lastNumber = 0;
    std::string lastText;
    std::string event;
    std::string message;

    void virtualWrite(int pin, float value) {
        writes++;
        lastPin = pin;
        lastNumber = value;
    }
    void virtualWrite(int pin, int value) { virtualWrite(pin, static_cast<float>(value)); }
    void virtualWrite(int pin, uint8_t value) { virtualWrite(pin, static_cast<float>(value)); }
    void virtualWrite(int pin, const char *value) {
        writes++;
        lastPin = pin;
        lastText = value;
    }
    void logEvent(const char *name, const std::string &description) {
        event = name;
        message = description;
    }
};

long fixedRandom(long minValue, long) { return minValue; }

int main() {
    TestClient client;
    TestOutput output;
    LoggedBlynk<TestClient, TestOutput> logged(client, output);

    logged.virtualWrite(0, 72.345f);
    logged.virtualWrite(10, "OFFLINE 01:23 PM");
    logged.virtualWrite(30, uint8_t(75));
    logged.virtualWrite(1, -999);
    logged.logEvent("node_offline", std::string("Node 1 offline"));
    assert(client.writes == 4 && client.lastPin == 1 && client.lastNumber == -999);
    assert(client.lastText == "OFFLINE 01:23 PM");
    assert(client.event == "node_offline" && client.message == "Node 1 offline");
    assert(output.stream.str() ==
           "[Blynk] V0 = 72.345\n"
           "[Blynk] V10 = OFFLINE 01:23 PM\n"
           "[Blynk] V30 = 75\n"
           "[Blynk] V1 = -999\n"
           "[Blynk] Event node_offline = Node 1 offline\n");

    output.stream.str("");
    output.stream.clear();
    DemoMode demo;
    demo.setEnabled(true);
    demo.publishIfDue(logged, 0, "ONLINE --:--", fixedRandom);
    std::string demoLog = output.stream.str();
    assert(client.writes == 27);
    assert(std::count(demoLog.begin(), demoLog.end(), '\n') == 23);
    assert(demoLog.find("[Blynk] V0 = 65.000\n") != std::string::npos);
    assert(demoLog.find("[Blynk] V10 = ONLINE --:--\n") != std::string::npos);
    assert(demoLog.find("[Blynk] V20 = 3.800\n") != std::string::npos);
    assert(demoLog.find("[Blynk] V30 = 75\n") != std::string::npos);
    assert(demoLog.find("[Blynk] V8 = 65.000\n") != std::string::npos);
}
