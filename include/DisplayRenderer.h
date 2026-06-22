#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "Max30100Snapshot.h"

class DisplayRenderer {
public:
    DisplayRenderer();

    void begin();
    void render(const Max30100Snapshot &snapshot);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_;
    uint32_t startedAtMs_ = 0;
    float smoothedHeartRateBpm_ = 0.0f;
    float smoothedSpo2_ = 0.0f;
    bool hasSmoothedHeartRate_ = false;
    bool hasSmoothedSpo2_ = false;

    void drawLine(uint8_t row, const char *text);
    void drawFingerPromptIcon();
    void drawSignalProgress(uint32_t beatCount);
    void updateSmoothedValues(const Max30100Snapshot &snapshot);
    void renderBootScreen();
    void renderSensorMissingScreen(const Max30100Snapshot &snapshot);
    void renderWaitingFingerScreen(const Max30100Snapshot &snapshot);
    void renderAcquiringSignalScreen(const Max30100Snapshot &snapshot);
    void renderMeasurementScreen(const Max30100Snapshot &snapshot);
};
