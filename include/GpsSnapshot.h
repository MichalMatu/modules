#pragma once

#include <Arduino.h>

struct GpsSnapshot {
    bool dataReceived = false;
    bool locationValid = false;
    bool freshFix = false;
    bool timeValid = false;
    bool dateValid = false;
    bool baudDetected = false;

    uint32_t uptimeMs = 0;
    uint32_t activeBaud = 0;
    uint32_t charsProcessed = 0;
    uint32_t passedChecksum = 0;
    uint32_t failedChecksum = 0;
    uint32_t locationAgeMs = UINT32_MAX;

    uint32_t satellites = 0;
    double hdop = 0.0;
    double latitude = 0.0;
    double longitude = 0.0;
    double speedKmph = 0.0;
    double altitudeMeters = 0.0;

    uint8_t hour = 0;
    uint8_t minute = 0;
    uint8_t second = 0;
    uint8_t day = 0;
    uint8_t month = 0;
    uint16_t year = 0;
};
