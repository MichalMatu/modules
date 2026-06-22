#pragma once

#include <Arduino.h>

struct Cc1101Snapshot {
    bool radioReady = false;
    bool listening = false;
    bool recentCode = false;

    uint32_t uptimeMs = 0;
    uint32_t lastInitAttemptAtMs = 0;
    uint32_t lastCodeAtMs = 0;
    uint32_t receivedCodeCount = 0;

    int16_t lastState = 0;
    float frequencyMhz = 0.0f;
    float rssiDbm = 0.0f;
    float noiseFloorDbm = 0.0f;
    float lastCodeRssiDbm = 0.0f;
    uint32_t lastValue = 0;
    uint16_t repeatCount = 0;
    uint16_t lastDelayUs = 0;
    uint8_t lastBitLength = 0;
    uint8_t lastProtocol = 0;
    char lastBinary[33] = {};
};
