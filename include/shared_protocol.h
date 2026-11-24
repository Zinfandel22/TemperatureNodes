#pragma once
#include <stdint.h>
#include <string.h>

constexpr uint32_t CONTROL_MAGIC = 0x544E5031;
constexpr uint8_t MAX_REMOTE_NODES = 4;

enum ControlType : uint8_t {
    PAIR_REQUEST = 1,
    PAIR_RESPONSE,
    RESET_PAIRING,
    RESET_ACK
};

struct ControlPacket {
    uint32_t magic;
    uint32_t nonce;
    uint8_t type;
    uint8_t nodeId;
    uint8_t nodeMac[6];
};

inline bool controlFromNode(const ControlPacket &packet, const uint8_t *sourceMac) {
    return packet.magic == CONTROL_MAGIC && packet.nodeId >= 1 &&
           packet.nodeId <= MAX_REMOTE_NODES && memcmp(packet.nodeMac, sourceMac, 6) == 0;
}

inline bool pairingResponseMatches(const ControlPacket &packet, uint8_t nodeId,
                                   const uint8_t *nodeMac, uint32_t nonce) {
    return packet.magic == CONTROL_MAGIC && packet.type == PAIR_RESPONSE &&
           packet.nodeId == nodeId && packet.nonce == nonce &&
           memcmp(packet.nodeMac, nodeMac, 6) == 0;
}

inline bool resetRequestMatches(const ControlPacket &packet, uint8_t nodeId,
                                const uint8_t *nodeMac, const uint8_t *sourceMac,
                                const uint8_t *hubMac) {
    return packet.magic == CONTROL_MAGIC && packet.type == RESET_PAIRING &&
           packet.nodeId == nodeId && memcmp(packet.nodeMac, nodeMac, 6) == 0 &&
           memcmp(sourceMac, hubMac, 6) == 0;
}

// ESP-NOW Protocol packet structure
struct SensorPacket {
    uint8_t  nodeId;           // Node ID (1-4)
    float    temperatureF;     // Temperature in Fahrenheit
    float    humidityPct;      // Relative humidity percentage
    float    batteryVolts;     // Battery voltage
    uint32_t bootCount;        // Boot counter for diagnostics
};

// Ensure packet structure doesn't have padding issues
static_assert(sizeof(SensorPacket) <= 250, "SensorPacket too large for ESP-NOW");
static_assert(sizeof(ControlPacket) != sizeof(SensorPacket), "ESP-NOW packet sizes must differ");
