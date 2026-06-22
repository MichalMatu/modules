#pragma once

#include <Arduino.h>

struct Max30100Snapshot {
    bool sensorFound = false;
    bool heartRateValid = false;
    bool spo2Valid = false;
    bool freshBeat = false;

    uint32_t uptimeMs = 0;
    uint32_t lastInitAttemptAtMs = 0;
    uint32_t lastBeatAtMs = 0;
    uint32_t beatCount = 0;

    float heartRateBpm = 0.0f;
    uint8_t spo2 = 0;
    uint8_t redLedCurrentBias = 0;
};
