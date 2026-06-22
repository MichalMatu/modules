#pragma once

#include <Arduino.h>

struct Cc1101Snapshot {
    bool radioReady = false;
    bool listening = false;
    bool recentBurst = false;
    bool lastDecoded = false;
    bool lastInverted = false;

    uint32_t uptimeMs = 0;
    uint32_t lastInitAttemptAtMs = 0;
    uint32_t lastBurstAtMs = 0;
    uint32_t burstCount = 0;
    uint32_t decodedBurstCount = 0;
    uint32_t rejectedBurstCount = 0;
    uint32_t edgeOverflowCount = 0;

    int16_t lastState = 0;
    float frequencyMhz = 0.0f;
    float rssiDbm = 0.0f;
    float noiseFloorDbm = 0.0f;
    float lastBurstRssiDbm = 0.0f;
    uint16_t lastPulseCount = 0;
    uint16_t lastUnitUs = 0;
    uint16_t lastBurstDurationMs = 0;
    uint16_t repeatCount = 0;
    uint8_t lastBitCount = 0;
    char lastBits[49] = {};
    char lastHex[17] = {};
    char rawPreview[97] = {};
};
