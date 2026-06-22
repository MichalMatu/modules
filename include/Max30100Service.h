#pragma once

#include <Arduino.h>
#include <MAX30100_PulseOximeter.h>

#include "Max30100Snapshot.h"

class Max30100Service {
public:
    Max30100Service();

    bool begin();
    void poll();
    bool snapshot(Max30100Snapshot &out, TickType_t waitTicks = pdMS_TO_TICKS(20));

private:
    PulseOximeter pulseOximeter_;
    SemaphoreHandle_t snapshotMutex_ = nullptr;
    Max30100Snapshot snapshot_;

    uint32_t beatCount_ = 0;
    uint32_t lastBeatAtMs_ = 0;
    uint32_t lastInitAttemptAtMs_ = 0;
    uint32_t lastPublishAtMs_ = 0;
    bool sensorFound_ = false;

    bool initializeSensor();
    void publishSnapshot();

    static Max30100Service *activeService_;
    static void handleBeatDetected();
};
