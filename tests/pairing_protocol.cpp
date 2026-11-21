#include "shared_protocol.h"
#include <cassert>

int main() {
    const uint8_t node[6] = {2, 1, 2, 3, 4, 5};
    const uint8_t other[6] = {2, 1, 2, 3, 4, 6};
    const uint8_t hub[6] = {4, 1, 2, 3, 4, 5};
    ControlPacket packet = {CONTROL_MAGIC, 42, PAIR_REQUEST, 2, {}};
    memcpy(packet.nodeMac, node, sizeof(node));

    assert(controlFromNode(packet, node));
    assert(!controlFromNode(packet, other));
    packet.magic = 0;
    assert(!controlFromNode(packet, node));
    packet.magic = CONTROL_MAGIC;
    packet.nodeId = 0;
    assert(!controlFromNode(packet, node));
    packet.nodeId = 5;
    assert(!controlFromNode(packet, node));

    packet.nodeId = 2;
    packet.type = PAIR_RESPONSE;
    assert(pairingResponseMatches(packet, 2, node, 42));
    assert(!pairingResponseMatches(packet, 2, node, 43));
    assert(!pairingResponseMatches(packet, 1, node, 42));
    assert(!pairingResponseMatches(packet, 2, other, 42));
    packet.type = PAIR_REQUEST;
    assert(!pairingResponseMatches(packet, 2, node, 42));

    packet.type = RESET_PAIRING;
    assert(resetRequestMatches(packet, 2, node, hub, hub));
    assert(!resetRequestMatches(packet, 2, node, other, hub));
    assert(!resetRequestMatches(packet, 2, other, hub, hub));
    assert(!resetRequestMatches(packet, 1, node, hub, hub));
    packet.type = RESET_ACK;
    assert(!resetRequestMatches(packet, 2, node, hub, hub));
    packet.type = RESET_PAIRING;
    packet.magic = 0;
    assert(!resetRequestMatches(packet, 2, node, hub, hub));
    return 0;
}
