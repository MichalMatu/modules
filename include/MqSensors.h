#pragma once

#include <Arduino.h>

class MqSensors {
public:
    enum SensorIndex : uint8_t {
        Mq2 = 0,
        Mq7,
        Mq9,
        SensorCount,
    };

    struct Reading {
        const char *name = "";
        uint8_t pin = 0;
        uint16_t raw = 0;
        uint8_t percent = 0;
        uint16_t minRaw = 0;
        uint16_t maxRaw = 0;
    };

    struct Snapshot {
        Reading readings[SensorCount];
        uint32_t sampleCount = 0;
        uint32_t lastSampleAtMs = 0;
        uint32_t lastSampleAgeMs = 0;
    };

    void begin();
    void update();
    void resetStats();
    Snapshot snapshot() const;

private:
    mutable portMUX_TYPE mux_ = portMUX_INITIALIZER_UNLOCKED;
    Reading readings_[SensorCount] = {
        {"MQ-2", 0, 0, 0, 0, 0},
        {"MQ-7", 0, 0, 0, 0, 0},
        {"MQ-9", 0, 0, 0, 0, 0},
    };
    uint32_t sampleCount_ = 0;
    uint32_t lastSampleAtMs_ = 0;

    static uint8_t rawToPercent(uint16_t raw);
    void updateStats(Reading &reading, uint16_t raw);
};
