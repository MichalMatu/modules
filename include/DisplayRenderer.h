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

    void drawLine(uint8_t row, const char *text);
    void drawFingerPromptIcon();
    void renderBootScreen();
    void renderSensorMissingScreen(const Max30100Snapshot &snapshot);
    void renderWaitingFingerScreen(const Max30100Snapshot &snapshot);
    void renderMeasurementScreen(const Max30100Snapshot &snapshot);
};
