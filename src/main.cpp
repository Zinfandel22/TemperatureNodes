#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include <esp_idf_version.h>
#include "shared_protocol.h"
#include "sensor_hal.h"
#include "node_pairing_reset.h"
#include "hub/BlynkSettings.h"
#include <BlynkEdgent.h>
#include "hub/Interaction.h"

constexpr uint8_t ROLE_PIN = D3;
bool isHub = false;

constexpr uint32_t HUB_LOCAL_INTERVAL = 30UL * 60UL * 1000UL;
constexpr uint32_t HUB_PAIRING_WINDOW = 2UL * 60UL * 1000UL;
constexpr uint32_t ELAPSED_TIME_INTERVAL = 60UL * 1000UL;
constexpr uint32_t NODE_OFFLINE_ATTEMPTS = 3;
constexpr uint32_t NODE_OFFLINE_MS = NODE_OFFLINE_ATTEMPTS * 30UL * 60UL * 1000UL;
constexpr uint8_t RESET_ALL_NODES = (1U << MAX_REMOTE_NODES) - 1;
const uint8_t BROADCAST_MAC[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

struct NodeSlot {
    bool hasNewData;
    bool hasContact;
    bool offlineAlertSent;
    uint32_t lastContactMs;
    SensorPacket packet;
    bool resetCheck;
    uint8_t sourceMac[6];
    bool pairRequest;
    uint32_t pairNonce;
    uint8_t pairingMac[6];
    bool resetAck;
    uint32_t resetAckNonce;
};

struct HubResetState {
    uint32_t generation;
    uint8_t pendingMask;
};

NodeSlot nodeSlots[MAX_REMOTE_NODES] = {};
portMUX_TYPE nodeSlotsMux = portMUX_INITIALIZER_UNLOCKED;
bool hasLocalSensor = false;
unsigned long lastLocalSampleMs = 0;
unsigned long lastLocalContactMs = 0;
bool hubEdgentReady = false;
bool hubRadioReady = false;
bool hubPairingOpen = false;
uint32_t hubPairingStarted = 0;
unsigned long lastElapsedPublishMs = 0;
Preferences hubPreferences;
bool hubResetStoreReady = false;
HubResetState hubResetState = {};

String formatElapsedHhMm(uint32_t elapsedMs) {
    uint32_t elapsedMinutes = elapsedMs / 60000UL;
    uint32_t hours = elapsedMinutes / 60UL;
    uint32_t minutes = elapsedMinutes % 60UL;

    char result[16];
    if (hours < 10) {
        snprintf(result, sizeof(result), "0%lu:%02lu",
                 static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes));
    } else {
        snprintf(result, sizeof(result), "%lu:%02lu",
                 static_cast<unsigned long>(hours),
                 static_cast<unsigned long>(minutes));
    }
    return String(result);
}

void onHubDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
    if (!recv_info || !recv_info->src_addr || !incomingData) return;
    if (len == sizeof(SensorPacket)) {
        SensorPacket tempPacket;
        memcpy(&tempPacket, incomingData, sizeof(tempPacket));
        if (tempPacket.nodeId < 1 || tempPacket.nodeId > MAX_REMOTE_NODES) return;
        uint8_t idx = tempPacket.nodeId - 1;
        portENTER_CRITICAL(&nodeSlotsMux);
        nodeSlots[idx].packet = tempPacket;
        nodeSlots[idx].hasNewData = true;
        nodeSlots[idx].hasContact = true;
        nodeSlots[idx].lastContactMs = millis();
        nodeSlots[idx].resetCheck = true;
        memcpy(nodeSlots[idx].sourceMac, recv_info->src_addr, 6);
        portEXIT_CRITICAL(&nodeSlotsMux);
    } else if (len == sizeof(ControlPacket)) {
        ControlPacket control;
        memcpy(&control, incomingData, sizeof(control));
        if (!controlFromNode(control, recv_info->src_addr)) return;
        uint8_t idx = control.nodeId - 1;
        portENTER_CRITICAL(&nodeSlotsMux);
        if (control.type == PAIR_REQUEST && hubPairingOpen &&
            millis() - hubPairingStarted < HUB_PAIRING_WINDOW) {
            nodeSlots[idx].pairRequest = true;
            nodeSlots[idx].pairNonce = control.nonce;
            memcpy(nodeSlots[idx].pairingMac, control.nodeMac, 6);
        } else if (control.type == RESET_ACK) {
            nodeSlots[idx].resetAck = true;
            nodeSlots[idx].resetAckNonce = control.nonce;
        }
        portEXIT_CRITICAL(&nodeSlotsMux);
    }
}

BLYNK_WRITE(V40) {
    int value = param.asInt();
    if (value == 1) {
        HubResetState next = {hubResetState.generation + 1, RESET_ALL_NODES};
        if (!hubResetStoreReady || hubPreferences.putBytes("state", &next, sizeof(next)) != sizeof(next)) {
            Serial.println("[Hub] Failed to save pairing reset request");
        } else {
            hubResetState = next;
            Serial.println("[Hub] Pairing reset queued for all nodes");
        }
        Blynk.virtualWrite(V40, 0);
    } else if (value != 0) {
        Serial.println("[Hub] V40 expects 0 or 1");
    }
}

bool clearResetForNode(uint8_t nodeId) {
    HubResetState next = hubResetState;
    next.pendingMask &= ~(1U << (nodeId - 1));
    if (next.pendingMask == hubResetState.pendingMask) return true;
    if (!hubResetStoreReady || hubPreferences.putBytes("state", &next, sizeof(next)) != sizeof(next)) {
        Serial.printf("[Hub] Failed to clear reset request for node %u\n", nodeId);
        return false;
    }
    hubResetState = next;
    return true;
}

void openHubPairing() {
    portENTER_CRITICAL(&nodeSlotsMux);
    bool ready = hubRadioReady;
    portEXIT_CRITICAL(&nodeSlotsMux);
    if (!ready || WiFi.status() != WL_CONNECTED || !Blynk.connected()) {
        Serial.println("[Hub] Pairing requires an active Wi-Fi and Blynk connection");
        return;
    }
    portENTER_CRITICAL(&nodeSlotsMux);
    for (auto &slot : nodeSlots) slot.pairRequest = false;
    hubPairingStarted = millis();
    hubPairingOpen = true;
    portEXIT_CRITICAL(&nodeSlotsMux);
    Serial.println("[Hub] Waiting for node pairing (window open for 2 minutes)");
}

BLYNK_WRITE(V41) {
    int value = param.asInt();
    if (value == 1) {
        openHubPairing();
        Blynk.virtualWrite(V41, 0);
    } else if (value != 0) {
        Serial.println("[Hub] V41 expects 0 or 1");
    }
}

void processHubControl() {
    portENTER_CRITICAL(&nodeSlotsMux);
    bool expired = hubPairingOpen && millis() - hubPairingStarted >= HUB_PAIRING_WINDOW;
    if (expired) hubPairingOpen = false;
    portEXIT_CRITICAL(&nodeSlotsMux);
    if (expired) Serial.println("[Hub] Pairing window closed");

    for (uint8_t i = 0; i < MAX_REMOTE_NODES; i++) {
        NodeSlot snapshot = {};
        portENTER_CRITICAL(&nodeSlotsMux);
        snapshot.resetAck = nodeSlots[i].resetAck;
        snapshot.resetAckNonce = nodeSlots[i].resetAckNonce;
        snapshot.resetCheck = nodeSlots[i].resetCheck;
        snapshot.pairRequest = nodeSlots[i].pairRequest && hubPairingOpen;
        snapshot.pairNonce = nodeSlots[i].pairNonce;
        memcpy(snapshot.sourceMac, nodeSlots[i].sourceMac, 6);
        memcpy(snapshot.pairingMac, nodeSlots[i].pairingMac, 6);
        nodeSlots[i].resetAck = false;
        nodeSlots[i].resetCheck = false;
        nodeSlots[i].pairRequest = false;
        portEXIT_CRITICAL(&nodeSlotsMux);

        uint8_t nodeId = i + 1;
        if (snapshot.resetAck && snapshot.resetAckNonce == hubResetState.generation &&
            (hubResetState.pendingMask & (1U << i)) && clearResetForNode(nodeId)) {
            Serial.printf("[Hub] Pairing cleared for node %u\n", nodeId);
        }

        if (snapshot.pairRequest && clearResetForNode(nodeId)) {
            ControlPacket response = {CONTROL_MAGIC, snapshot.pairNonce, PAIR_RESPONSE, nodeId, {}};
            memcpy(response.nodeMac, snapshot.pairingMac, 6);
            esp_err_t result = esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&response), sizeof(response));
            if (result != ESP_OK) {
                Serial.printf("[Hub] Pairing response for node %u failed: %s\n", nodeId, esp_err_to_name(result));
            }
        }

        if (snapshot.resetCheck && (hubResetState.pendingMask & (1U << i))) {
            ControlPacket reset = {CONTROL_MAGIC, hubResetState.generation, RESET_PAIRING, nodeId, {}};
            memcpy(reset.nodeMac, snapshot.sourceMac, 6);
            esp_err_t result = esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&reset), sizeof(reset));
            if (result != ESP_OK) {
                Serial.printf("[Hub] Reset request for node %u failed: %s\n", nodeId, esp_err_to_name(result));
            }
        }
    }
}

uint8_t lipoVoltsToPercent(float volts) {
    if (volts >= 4.20f) return 100;
    if (volts <= 3.20f) return 0;
    return static_cast<uint8_t>(((volts - 3.20f) / (4.20f - 3.20f)) * 100.0f);
}

void onHubWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
    static bool espNowInitialized = false;

    if (event == ARDUINO_EVENT_WIFI_STA_START) {
        if (!WiFi.setSleep(false)) {
            Serial.println("[Hub] Failed to disable Wi-Fi sleep");
        }
        esp_err_t result = esp_now_init();
        if (result == ESP_OK) {
            espNowInitialized = true;
            result = esp_now_register_recv_cb(onHubDataRecv);
        }
        if (result == ESP_OK) {
            esp_now_peer_info_t peer = {};
            memcpy(peer.peer_addr, BROADCAST_MAC, 6);
            peer.ifidx = WIFI_IF_STA;
            result = esp_now_add_peer(&peer);
        }
        portENTER_CRITICAL(&nodeSlotsMux);
        hubRadioReady = (result == ESP_OK);
        portEXIT_CRITICAL(&nodeSlotsMux);
        if (result != ESP_OK) {
            Serial.printf("[Hub] ESP-NOW initialization failed: %s\n", esp_err_to_name(result));
        }
    } else if (event == ARDUINO_EVENT_WIFI_STA_STOP && espNowInitialized) {
        portENTER_CRITICAL(&nodeSlotsMux);
        hubRadioReady = false;
        hubPairingOpen = false;
        portEXIT_CRITICAL(&nodeSlotsMux);
        esp_err_t result = esp_now_deinit();
        if (result != ESP_OK) {
            Serial.printf("[Hub] ESP-NOW shutdown failed: %s\n", esp_err_to_name(result));
        }
        espNowInitialized = false;
    } else if (event == ARDUINO_EVENT_WIFI_STA_CONNECTED) {
        uint8_t channel = info.wifi_sta_connected.channel;
        Serial.println("[Hub] Wi-Fi connected to access point; waiting for IP address");
        Serial.printf("[Hub] Station MAC: %s, Wi-Fi channel: %u\n", WiFi.macAddress().c_str(), channel);
    } else if (event == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
        Serial.println("[Hub] Wi-Fi connected and IP address acquired");
    } else if (event == ARDUINO_EVENT_WIFI_STA_DISCONNECTED) {
        portENTER_CRITICAL(&nodeSlotsMux);
        hubPairingOpen = false;
        portEXIT_CRITICAL(&nodeSlotsMux);
        Serial.printf("[Hub] Wi-Fi disconnected or connection failed (reason %u)\n",
                      static_cast<unsigned>(info.wifi_sta_disconnected.reason));
    }
}

void setupHub() {
    Serial.begin(115200);
    delay(200);
    Serial.println("[Hub] Starting");

    hasLocalSensor = initSht40();
    hubResetStoreReady = hubPreferences.begin("hub-reset", false);
    if (hubResetStoreReady && hubPreferences.isKey("state")) {
        if (hubPreferences.getBytesLength("state") != sizeof(hubResetState) ||
            hubPreferences.getBytes("state", &hubResetState, sizeof(hubResetState)) != sizeof(hubResetState) ||
            (hubResetState.pendingMask & ~RESET_ALL_NODES) != 0) {
            hubResetStoreReady = false;
            Serial.println("[Hub] Invalid stored pairing reset state");
        }
    }
    if (!hubResetStoreReady) {
        Serial.println("[Hub] Pairing reset storage unavailable");
    }
    interaction.onPairingRequest(openHubPairing);
    interaction.begin();
    WiFi.onEvent(onHubWiFiEvent);
    BlynkEdgent.onStateChange([]() {
        BLYNK_LOG("State: %s", BlynkEdgent.getStateName());
        switch (BlynkEdgent.getState()) {
        case Edgent::MODE_WAIT_CONFIG:
            Serial.println("[Hub] Waiting for Blynk provisioning");
            break;
        case Edgent::MODE_CONNECTING_NET:
            Serial.println("[Hub] Wi-Fi connecting");
            break;
        case Edgent::MODE_CONNECTING_CLOUD:
            Serial.println("[Hub] Connecting to Blynk");
            break;
        case Edgent::MODE_RUNNING:
            Serial.println("[Hub] Blynk connected; open a pairing window with V41 or the D0 button");
            break;
        case Edgent::MODE_ERROR:
            Serial.println("[Hub] Blynk Edgent connection error; see preceding logs");
            break;
        default:
            break;
        }
        interaction.updateIndicator();
    });
    Serial.println("[Hub] Starting Blynk Edgent");
    hubEdgentReady = BlynkEdgent.begin();
    if (!hubEdgentReady) {
        Serial.println("[Hub] Blynk Edgent initialization failed");
    }
    lastLocalSampleMs = millis() - HUB_LOCAL_INTERVAL;
    lastElapsedPublishMs = millis() - ELAPSED_TIME_INTERVAL;
}

void loopHub() {
    if (!hubEdgentReady) {
        delay(1000);
        return;
    }
    BlynkEdgent.run();
    interaction.run();
    processHubControl();
    if (!Blynk.connected()) return;

    for (uint8_t i = 0; i < MAX_REMOTE_NODES; i++) {
        SensorPacket pkt;
        portENTER_CRITICAL(&nodeSlotsMux);
        bool hasNewData = nodeSlots[i].hasNewData;
        if (hasNewData) {
            memcpy(&pkt, &nodeSlots[i].packet, sizeof(SensorPacket));
            nodeSlots[i].hasNewData = false;
            nodeSlots[i].offlineAlertSent = false;
        }
        portEXIT_CRITICAL(&nodeSlotsMux);

        if (hasNewData) {
            Blynk.virtualWrite(i,      pkt.temperatureC);
            Blynk.virtualWrite(i + 4,  pkt.humidityPct);
            Blynk.virtualWrite(i + 10, "00:00");
            Blynk.virtualWrite(i + 20, pkt.batteryVolts);
            Blynk.virtualWrite(i + 30, lipoVoltsToPercent(pkt.batteryVolts));

            if (pkt.batteryVolts > 2.50f && pkt.batteryVolts < 3.40f) {
                Blynk.logEvent("low_battery", String("Node ") + pkt.nodeId + " low: " + String(pkt.batteryVolts, 2) + "V");
            }
        }
    }

    if (millis() - lastElapsedPublishMs >= ELAPSED_TIME_INTERVAL) {
        lastElapsedPublishMs = millis();
        for (uint8_t i = 0; i < MAX_REMOTE_NODES; i++) {
            portENTER_CRITICAL(&nodeSlotsMux);
            bool hasContact = nodeSlots[i].hasContact;
            uint32_t lastContactMs = nodeSlots[i].lastContactMs;
            bool offlineAlertSent = nodeSlots[i].offlineAlertSent;
            portEXIT_CRITICAL(&nodeSlotsMux);

            if (hasContact) {
                uint32_t elapsedMs = millis() - lastContactMs;
                if (elapsedMs >= NODE_OFFLINE_MS) {
                    Blynk.virtualWrite(i + 10,
                                       "OFFLINE " + formatElapsedHhMm(elapsedMs));
                    if (!offlineAlertSent) {
                        portENTER_CRITICAL(&nodeSlotsMux);
                        if (!nodeSlots[i].offlineAlertSent) {
                            nodeSlots[i].offlineAlertSent = true;
                            offlineAlertSent = true;
                        }
                        portEXIT_CRITICAL(&nodeSlotsMux);
                        if (offlineAlertSent) {
                            Blynk.logEvent("node_offline",
                                           String("Node ") + (i + 1) +
                                           " offline after " + NODE_OFFLINE_ATTEMPTS +
                                           " missed reports");
                        }
                    }
                } else {
                    Blynk.virtualWrite(i + 10, formatElapsedHhMm(elapsedMs));
                }
            } else {
                Blynk.virtualWrite(i + 10, "--:--");
            }
        }

        if (hasLocalSensor) {
            portENTER_CRITICAL(&nodeSlotsMux);
            uint32_t elapsedMs = lastLocalContactMs ? millis() - lastLocalContactMs : 0;
            portEXIT_CRITICAL(&nodeSlotsMux);
            Blynk.virtualWrite(V14, formatElapsedHhMm(elapsedMs));
        }
    }

    if (hasLocalSensor && (millis() - lastLocalSampleMs >= HUB_LOCAL_INTERVAL)) {
        lastLocalSampleMs = millis();
        ShtReading localEnv = readSht40();
        lastLocalContactMs = millis();
        Blynk.virtualWrite(V8,  localEnv.temperatureC);
        Blynk.virtualWrite(V9,  localEnv.humidityPct);
        Blynk.virtualWrite(V14, "00:00");
    }
}

constexpr uint8_t  ID_PIN_BIT0        = D1; // GPIO 2
constexpr uint8_t  ID_PIN_BIT1        = D2; // GPIO 3
constexpr uint64_t NODE_SLEEP_SECONDS = 30 * 60;
constexpr uint8_t  MAX_SEND_RETRIES   = 2;
constexpr uint32_t PAIR_DISCOVERY_TIME = 30000;
constexpr uint32_t PAIR_CHANNEL_TIME = 450;
constexpr uint32_t RESET_RESPONSE_TIME = 1000;

struct StoredHub {
    uint32_t magic;
    uint8_t mac[6];
    uint8_t channel;
    uint8_t version;
};

RTC_DATA_ATTR uint32_t rtcBootCount = 0;
bool deliveryCallbackFired = false;
bool deliverySuccess = false;
portMUX_TYPE deliveryMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE nodeControlMux = portMUX_INITIALIZER_UNLOCKED;
StoredHub nodeHub = {};
uint8_t nodeLocalMac[6] = {};
uint8_t nodeIdForControl = 0;
uint32_t nodePairNonce = 0;
bool nodePaired = false;
bool nodePairFound = false;
bool nodeResetReceived = false;
uint32_t nodeResetNonce = 0;

void onNodeDataRecv(const esp_now_recv_info *info, const uint8_t *data, int len) {
    if (!info || !info->src_addr || !info->rx_ctrl || !data || len != sizeof(ControlPacket)) return;
    ControlPacket control;
    memcpy(&control, data, sizeof(control));

    portENTER_CRITICAL(&nodeControlMux);
    if (!nodePaired && !nodePairFound &&
        pairingResponseMatches(control, nodeIdForControl, nodeLocalMac, nodePairNonce) &&
        !(info->src_addr[0] & 1) &&
        info->rx_ctrl->channel >= 1 && info->rx_ctrl->channel <= 14) {
        nodeHub.magic = CONTROL_MAGIC;
        memcpy(nodeHub.mac, info->src_addr, 6);
        nodeHub.channel = info->rx_ctrl->channel;
        nodeHub.version = 1;
        nodePairFound = true;
    } else if (nodePaired && resetRequestMatches(control, nodeIdForControl, nodeLocalMac,
                                                 info->src_addr, nodeHub.mac)) {
        nodeResetReceived = true;
        nodeResetNonce = control.nonce;
    }
    portEXIT_CRITICAL(&nodeControlMux);
}

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 5, 0)
void onNodeDataSent(const wifi_tx_info_t *tx_info, esp_now_send_status_t status) {
    if (!tx_info || !tx_info->des_addr || memcmp(tx_info->des_addr, nodeHub.mac, 6) != 0) return;
#else
void onNodeDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    if (!mac_addr || memcmp(mac_addr, nodeHub.mac, 6) != 0) return;
#endif
    portENTER_CRITICAL(&deliveryMux);
    deliverySuccess = (status == ESP_NOW_SEND_SUCCESS);
    deliveryCallbackFired = true;
    portEXIT_CRITICAL(&deliveryMux);
}

uint8_t readHardwareNodeId() {
    pinMode(ID_PIN_BIT0, INPUT_PULLUP);
    pinMode(ID_PIN_BIT1, INPUT_PULLUP);
    delayMicroseconds(50);

    uint8_t bit0 = (digitalRead(ID_PIN_BIT0) == LOW) ? 1 : 0;
    uint8_t bit1 = (digitalRead(ID_PIN_BIT1) == LOW) ? 1 : 0;

    pinMode(ID_PIN_BIT0, INPUT);
    pinMode(ID_PIN_BIT1, INPUT);
    return ((bit1 << 1) | bit0) + 1;
}

void enterNodeDeepSleep() {
    Serial.println("[Node] Sleeping for 30 minutes");
    Serial.flush();
    esp_wifi_stop();
    esp_sleep_enable_timer_wakeup(NODE_SLEEP_SECONDS * 1000000ULL);
    esp_deep_sleep_start();
}

bool setNodeChannel(uint8_t channel) {
    esp_err_t result = esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    if (result != ESP_OK) {
        Serial.printf("[Node] Cannot set channel %u: %s\n", channel, esp_err_to_name(result));
        return false;
    }
    return true;
}

bool discoverHub(uint8_t nodeId) {
    wifi_country_t country = {};
    esp_err_t result = esp_wifi_get_country(&country);
    if (result != ESP_OK || country.nchan == 0) {
        Serial.printf("[Node] Cannot read Wi-Fi channel range: %s\n", esp_err_to_name(result));
        return false;
    }

    ControlPacket request = {CONTROL_MAGIC, nodePairNonce, PAIR_REQUEST, nodeId, {}};
    memcpy(request.nodeMac, nodeLocalMac, 6);
    uint32_t started = millis();
    while (millis() - started < PAIR_DISCOVERY_TIME) {
        for (uint8_t offset = 0; offset < country.nchan && millis() - started < PAIR_DISCOVERY_TIME; offset++) {
            uint8_t channel = country.schan + offset;
            if (!setNodeChannel(channel)) continue;
            uint32_t channelStarted = millis();
            do {
                result = esp_now_send(BROADCAST_MAC, reinterpret_cast<const uint8_t *>(&request), sizeof(request));
                if (result != ESP_OK) {
                    Serial.printf("[Node] Discovery send failed: %s\n", esp_err_to_name(result));
                }
                for (uint8_t i = 0; i < 15; i++) {
                    delay(10);
                    portENTER_CRITICAL(&nodeControlMux);
                    bool found = nodePairFound;
                    portEXIT_CRITICAL(&nodeControlMux);
                    if (found) return true;
                }
            } while (millis() - channelStarted < PAIR_CHANNEL_TIME &&
                     millis() - started < PAIR_DISCOVERY_TIME);
        }
    }
    Serial.println("[Node] No hub found; retrying after deep sleep");
    return false;
}

bool sendNodePacket(const uint8_t *data, size_t length) {
    for (uint8_t attempt = 0; attempt <= MAX_SEND_RETRIES; attempt++) {
        portENTER_CRITICAL(&deliveryMux);
        deliveryCallbackFired = false;
        deliverySuccess = false;
        portEXIT_CRITICAL(&deliveryMux);

        esp_err_t result = esp_now_send(nodeHub.mac, data, length);
        if (result == ESP_OK) {
            unsigned long startWait = millis();
            bool callbackFired;
            bool success;
            do {
                portENTER_CRITICAL(&deliveryMux);
                callbackFired = deliveryCallbackFired;
                success = deliverySuccess;
                portEXIT_CRITICAL(&deliveryMux);
                if (!callbackFired) delay(1);
            } while (!callbackFired && (millis() - startWait < 50));
            if (callbackFired && success) return true;
        } else {
            Serial.printf("[Node] ESP-NOW send failed: %s\n", esp_err_to_name(result));
        }
        delay(random(10, 40));
    }
    Serial.println("[Node] ESP-NOW delivery failed");
    return false;
}

void setupNode() {
    Serial.begin(115200);
    rtcBootCount++;

    uint8_t nodeId = readHardwareNodeId();
    Serial.printf("[Node %u] Starting\n", nodeId);
    Preferences preferences;
    if (!preferences.begin("node-pair", false)) {
        Serial.println("[Node] Pairing storage unavailable");
        enterNodeDeepSleep();
    }
    if (!handleNodePairingReset(preferences)) {
        preferences.end();
        enterNodeDeepSleep();
    }
    Serial.println("[Node] Reading battery and SHT40");
    float battVolts = readBatteryVolts();
    if (!initSht40()) {
        Serial.println("[Node] SHT40 initialization failed; sending invalid sensor readings");
    }
    ShtReading env = readSht40();

    if (preferences.isKey("hub")) {
        if (preferences.getBytesLength("hub") != sizeof(nodeHub) ||
            preferences.getBytes("hub", &nodeHub, sizeof(nodeHub)) != sizeof(nodeHub) ||
            nodeHub.magic != CONTROL_MAGIC || nodeHub.version != 1 ||
            nodeHub.channel < 1 || nodeHub.channel > 14 || (nodeHub.mac[0] & 1)) {
            Serial.println("[Node] Invalid stored pairing; clear node storage to re-pair");
            enterNodeDeepSleep();
        }
        nodePaired = true;
        Serial.printf("[Node] Using saved hub on channel %u\n", nodeHub.channel);
    }

    if (!WiFi.mode(WIFI_STA)) {
        Serial.println("[Node] Failed to start Wi-Fi station");
        enterNodeDeepSleep();
    }
    WiFi.disconnect();

    esp_err_t result = esp_wifi_get_mac(WIFI_IF_STA, nodeLocalMac);
    if (result == ESP_OK) result = esp_now_init();
    if (result == ESP_OK) result = esp_now_register_recv_cb(onNodeDataRecv);
    if (result != ESP_OK) {
        Serial.printf("[Node] ESP-NOW initialization failed: %s\n", esp_err_to_name(result));
        enterNodeDeepSleep();
    }

    nodeIdForControl = nodeId;
    if (!nodePaired) {
        esp_now_peer_info_t broadcastPeer = {};
        memcpy(broadcastPeer.peer_addr, BROADCAST_MAC, 6);
        broadcastPeer.ifidx = WIFI_IF_STA;
        result = esp_now_add_peer(&broadcastPeer);
        if (result != ESP_OK) {
            Serial.printf("[Node] Cannot add discovery peer: %s\n", esp_err_to_name(result));
            enterNodeDeepSleep();
        }
        nodePairNonce = esp_random();
        Serial.println("[Node] Searching for a hub (up to 30 seconds)");
        if (!discoverHub(nodeId)) enterNodeDeepSleep();
        if (preferences.putBytes("hub", &nodeHub, sizeof(nodeHub)) != sizeof(nodeHub)) {
            Serial.println("[Node] Failed to save hub pairing");
            enterNodeDeepSleep();
        }
        portENTER_CRITICAL(&nodeControlMux);
        nodePaired = true;
        portEXIT_CRITICAL(&nodeControlMux);
        Serial.printf("[Node] Paired with hub on channel %u\n", nodeHub.channel);
    }

    if (!setNodeChannel(nodeHub.channel)) enterNodeDeepSleep();
    result = esp_now_register_send_cb(onNodeDataSent);
    if (result != ESP_OK) {
        Serial.printf("[Node] Cannot register send callback: %s\n", esp_err_to_name(result));
        enterNodeDeepSleep();
    }

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, nodeHub.mac, 6);
    peerInfo.channel = nodeHub.channel;
    peerInfo.ifidx = WIFI_IF_STA;
    peerInfo.encrypt = false;

    result = esp_now_add_peer(&peerInfo);
    if (result != ESP_OK) {
        Serial.printf("[Node] Cannot add hub peer: %s\n", esp_err_to_name(result));
        enterNodeDeepSleep();
    }

    SensorPacket packet = {
        .nodeId       = nodeId,
        .temperatureC = env.temperatureC,
        .humidityPct  = env.humidityPct,
        .batteryVolts = battVolts,
        .bootCount    = rtcBootCount
    };

    if (sendNodePacket(reinterpret_cast<const uint8_t *>(&packet), sizeof(packet))) {
        Serial.println("[Node] Measurement acknowledged at radio level");
    }

    uint32_t startWait = millis();
    bool resetRequested = false;
    uint32_t resetNonce = 0;
    while (millis() - startWait < RESET_RESPONSE_TIME) {
        portENTER_CRITICAL(&nodeControlMux);
        resetRequested = nodeResetReceived;
        resetNonce = nodeResetNonce;
        portEXIT_CRITICAL(&nodeControlMux);
        if (resetRequested) break;
        delay(10);
    }
    if (resetRequested) {
        if (preferences.remove("hub")) {
            ControlPacket ack = {CONTROL_MAGIC, resetNonce, RESET_ACK, nodeId, {}};
            memcpy(ack.nodeMac, nodeLocalMac, 6);
            sendNodePacket(reinterpret_cast<const uint8_t *>(&ack), sizeof(ack));
            Serial.println("[Node] Pairing cleared; open a hub pairing window to re-pair");
        } else {
            Serial.println("[Node] Failed to clear stored pairing");
        }
    }
    preferences.end();
    enterNodeDeepSleep();
}

void setup() {
    pinMode(ROLE_PIN, INPUT_PULLUP);
    delayMicroseconds(50);
    isHub = digitalRead(ROLE_PIN) == LOW;
    pinMode(ROLE_PIN, INPUT);

    if (isHub) {
        setupHub();
    } else {
        setupNode();
    }
}

void loop() {
    if (isHub) {
        loopHub();
    }
}