#pragma once

#include <Arduino.h>
#include <RadioLib.h>

#include "Cc1101Snapshot.h"

class Cc1101Service {
public:
    Cc1101Service();

    bool begin();
    void poll();
    bool snapshot(Cc1101Snapshot &out, TickType_t waitTicks = pdMS_TO_TICKS(20));

private:
    static constexpr size_t MaxBurstPulses = 160;
    static constexpr size_t MaxDecodedBits = 48;

    struct SniffPulse {
        uint16_t durationUs = 0;
        uint8_t level = 0;
    };

    struct DecodeResult {
        bool decoded = false;
        bool inverted = false;
        uint8_t bitCount = 0;
        uint16_t unitUs = 0;
        char bits[MaxDecodedBits + 1] = {};
        char hex[17] = {};
    };

    Module module_;
    CC1101 radio_;
    SemaphoreHandle_t snapshotMutex_ = nullptr;
    Cc1101Snapshot snapshot_;

    SniffPulse burst_[MaxBurstPulses];
    size_t burstPulseCount_ = 0;
    float burstPeakRssiDbm_ = -140.0f;
    bool burstOverflowed_ = false;

    uint32_t lastInitAttemptAtMs_ = 0;
    uint32_t lastReceiveAttemptAtMs_ = 0;
    uint32_t lastBurstPulseAtMs_ = 0;
    uint32_t lastBurstAtMs_ = 0;
    uint32_t burstCount_ = 0;
    uint32_t decodedBurstCount_ = 0;
    uint32_t rejectedBurstCount_ = 0;
    uint32_t edgeOverflowCount_ = 0;
    int16_t lastState_ = 0;
    bool radioReady_ = false;
    bool listening_ = false;
    bool captureAttached_ = false;
    bool lastDecoded_ = false;
    bool lastInverted_ = false;
    float lastRssiDbm_ = -120.0f;
    float noiseFloorDbm_ = -120.0f;
    float lastBurstRssiDbm_ = -120.0f;
    uint16_t lastPulseCount_ = 0;
    uint16_t lastUnitUs_ = 0;
    uint16_t lastBurstDurationMs_ = 0;
    uint16_t repeatCount_ = 0;
    uint8_t lastBitCount_ = 0;
    char lastBits_[MaxDecodedBits + 1] = {};
    char lastHex_[17] = {};
    char rawPreview_[97] = {};

    bool initializeRadio();
    void processCapturedPulses();
    bool popCapturedPulse(uint8_t &level, uint16_t &durationUs);
    void handlePulse(uint8_t level, uint16_t durationUs);
    void finalizeBurst();
    bool decodeBurst(DecodeResult &result) const;
    void buildRawPreview(char *out, size_t outSize) const;
    void publishSnapshot();
    void refreshRssi();
    void restartReceive();

    static void handleGdo0Change();
};
