#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

#include "GpsSnapshot.h"

class GpsService {
public:
    GpsService();

    bool begin();
    void poll();
    bool snapshot(GpsSnapshot &out, TickType_t waitTicks = pdMS_TO_TICKS(20));

private:
    HardwareSerial serial_;
    TinyGPSPlus parser_;
    SemaphoreHandle_t snapshotMutex_ = nullptr;
    GpsSnapshot snapshot_;
    uint32_t activeBaud_ = 0;
    uint32_t lastBaudScanAtMs_ = 0;
    bool baudDetected_ = false;

    bool autoDetectBaud();
    bool probeBaud(uint32_t baud);
    void beginSerial(uint32_t baud);
    bool isNmeaSentence(const char *sentence) const;
    void publishSnapshot();
};
