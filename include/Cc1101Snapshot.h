#pragma once

#include <Arduino.h>

struct Cc1101Snapshot {
    bool radioReady = false;
    bool listening = false;
    bool recentPacket = false;

    uint32_t uptimeMs = 0;
    uint32_t lastInitAttemptAtMs = 0;
    uint32_t lastPacketAtMs = 0;
    uint32_t packetCount = 0;
    uint32_t crcErrorCount = 0;
    uint32_t receiveErrorCount = 0;

    int16_t lastState = 0;
    float frequencyMhz = 0.0f;
    float rssiDbm = 0.0f;
    uint8_t lqi = 0;
    uint8_t lastPacketLength = 0;
    char lastPacketHex[33] = {};
};
