#include <cassert>
#include "Arduino.h"
#include "node_pairing_reset.h"

TestSerial Serial;
uint32_t nowMs = 0;
uint32_t releaseMs = 0;
uint32_t adcMillivolts = 0;
int lastPinMode = INPUT;

void pinMode(int, int mode) { lastPinMode = mode; }
void delay(uint32_t ms) { nowMs += ms; }
uint32_t millis() { return nowMs; }
int digitalRead(int) { return nowMs < releaseMs ? LOW : HIGH; }
void analogSetAttenuation(int) {}
uint32_t analogReadMilliVolts(int) { return adcMillivolts; }

void startScenario(uint32_t release, uint32_t voltage) {
    nowMs = 0;
    releaseMs = release;
    adcMillivolts = voltage;
    lastPinMode = INPUT;
}

int main() {
    Preferences normal;
    startScenario(0, 0);
    assert(handleNodePairingReset(normal));
    assert(normal.hasHub && normal.removals == 0 && lastPinMode == INPUT);

    Preferences divider;
    startScenario(30000, 1600);
    assert(handleNodePairingReset(divider));
    assert(divider.hasHub && divider.removals == 0 && nowMs < 1000);

    Preferences shortPress;
    startScenario(5000, 0);
    assert(handleNodePairingReset(shortPress));
    assert(shortPress.hasHub && shortPress.removals == 0);

    Preferences longPress;
    startScenario(12000, 0);
    assert(handleNodePairingReset(longPress));
    assert(!longPress.hasHub && longPress.removals == 1 && lastPinMode == INPUT);

    Preferences minimumHold;
    startScenario(10060, 0);
    assert(handleNodePairingReset(minimumHold));
    assert(!minimumHold.hasHub);

    Preferences justShort;
    startScenario(10050, 0);
    assert(handleNodePairingReset(justShort));
    assert(justShort.hasHub);

    Preferences maximumHold;
    startScenario(20060, 0);
    assert(handleNodePairingReset(maximumHold));
    assert(maximumHold.hasHub);

    Preferences canceled;
    startScenario(30000, 0);
    assert(!handleNodePairingReset(canceled));
    assert(canceled.hasHub && canceled.removals == 0 && lastPinMode == INPUT);

    Preferences failure;
    failure.removeSucceeds = false;
    startScenario(12000, 0);
    assert(!handleNodePairingReset(failure));
    assert(failure.hasHub && failure.removals == 1);

    Preferences unpaired;
    unpaired.hasHub = false;
    startScenario(30000, 0);
    assert(handleNodePairingReset(unpaired));
    assert(unpaired.removals == 0 && nowMs == 0);
}
