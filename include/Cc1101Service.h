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
    Module module_;
    CC1101 radio_;
    SemaphoreHandle_t snapshotMutex_ = nullptr;
    Cc1101Snapshot snapshot_;

    uint32_t lastInitAttemptAtMs_ = 0;
    uint32_t lastReceiveAttemptAtMs_ = 0;
    uint32_t lastPacketAtMs_ = 0;
    uint32_t packetCount_ = 0;
    uint32_t crcErrorCount_ = 0;
    uint32_t receiveErrorCount_ = 0;
    int16_t lastState_ = 0;
    bool radioReady_ = false;
    bool listening_ = false;
    float lastRssiDbm_ = 0.0f;
    uint8_t lastLqi_ = 0;
    uint8_t lastPacketLength_ = 0;
    char lastPacketHex_[33] = {};

    bool initializeRadio();
    void handleReceivedPacket();
    void publishSnapshot();
    void refreshRssi();
    void restartReceive();

    static volatile bool packetReceived_;
    static void handlePacketReceived();
};
