#include "node_pairing_reset.h"
#include <Arduino.h>
#include "hub/BoardConfig.h"

constexpr uint32_t BUTTON_SETTLE_MS = 20;
constexpr uint32_t BATTERY_SETTLE_MS = 60;
constexpr uint32_t BUTTON_MAX_MILLIVOLTS = 250;

bool handleNodePairingReset(Preferences &preferences) {
    if (!preferences.isKey("hub")) return true;

    pinMode(BOARD_BUTTON_PIN, INPUT_PULLUP);
    delay(BUTTON_SETTLE_MS);
    bool pressed = digitalRead(BOARD_BUTTON_PIN) == LOW;
    if (pressed) {
        pinMode(BOARD_BUTTON_PIN, INPUT);
        delay(BUTTON_SETTLE_MS);
        analogSetAttenuation(ADC_11db);
        (void)analogReadMilliVolts(BOARD_BUTTON_PIN);
        uint32_t millivolts = analogReadMilliVolts(BOARD_BUTTON_PIN);
        pinMode(BOARD_BUTTON_PIN, INPUT_PULLUP);
        delay(BUTTON_SETTLE_MS);
        pressed = millivolts < BUTTON_MAX_MILLIVOLTS &&
                  digitalRead(BOARD_BUTTON_PIN) == LOW;
    }

    bool resetRequested = false;
    bool heldTooLong = false;
    if (pressed) {
        Serial.println("[Node] Hold D0 for 10 seconds, then release to clear pairing");
        uint32_t started = millis();
        bool releasePrompted = false;
        while (digitalRead(BOARD_BUTTON_PIN) == LOW) {
            uint32_t held = millis() - started;
            if (held >= BUTTON_HOLD_TIME_CANCEL) {
                heldTooLong = true;
                break;
            }
            if (held >= BUTTON_HOLD_TIME_CONFIG_RESET && !releasePrompted) {
                Serial.println("[Node] Release D0 to clear pairing");
                releasePrompted = true;
            }
            delay(10);
        }
        uint32_t held = millis() - started;
        resetRequested = !heldTooLong && held >= BUTTON_HOLD_TIME_CONFIG_RESET &&
                         held < BUTTON_HOLD_TIME_CANCEL;
        if (!resetRequested) Serial.println("[Node] Pairing reset canceled");
    }

    pinMode(BOARD_BUTTON_PIN, INPUT);
    delay(BATTERY_SETTLE_MS);
    if (heldTooLong) return false;
    if (resetRequested) {
        if (!preferences.remove("hub")) {
            Serial.println("[Node] Failed to clear stored pairing");
            return false;
        }
        Serial.println("[Node] Pairing cleared; searching for a hub");
    }
    return true;
}
