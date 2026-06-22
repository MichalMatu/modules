#pragma once

#include <Arduino.h>
#include <RadioLib.h>
#include <RCSwitch.h>

#include "Cc1101Snapshot.h"

class Cc1101Service {
public:
    Cc1101Service();

    bool begin();
    void poll();
    bool snapshot(Cc1101Snapshot &out, TickType_t waitTicks = pdMS_TO_TICKS(20));

private:
    Module module_;
    CC1101 radio_;
    RCSwitch rcSwitch_;
    SemaphoreHandle_t snapshotMutex_ = nullptr;
    Cc1101Snapshot snapshot_;

    uint32_t lastInitAttemptAtMs_ = 0;
    uint32_t lastReceiveAttemptAtMs_ = 0;
    uint32_t lastCodeAtMs_ = 0;
    uint32_t receivedCodeCount_ = 0;
    int16_t lastState_ = 0;
    bool radioReady_ = false;
    bool listening_ = false;
    bool rcSwitchEnabled_ = false;
    float lastRssiDbm_ = -120.0f;
    float noiseFloorDbm_ = -120.0f;
    float lastCodeRssiDbm_ = -120.0f;
    unsigned long lastValue_ = 0;
    uint16_t repeatCount_ = 0;
    uint16_t lastDelayUs_ = 0;
    uint8_t lastBitLength_ = 0;
    uint8_t lastProtocol_ = 0;
    char lastBinary_[33] = {};
    char rawPreview_[97] = {};

    bool initializeRadio();
    void handleReceivedCode();
    void buildRawPreview(const unsigned int *rawTimings, unsigned int bitLength);
    void publishSnapshot();
    void refreshRssi();
    void restartReceive();
};
