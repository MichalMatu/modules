#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "GpsSnapshot.h"

class DisplayRenderer {
public:
    DisplayRenderer();

    void begin();
    void renderBootScreen();
    void render(const GpsSnapshot &snapshot);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_;
    uint32_t startedAtMs_ = 0;

    void drawLine(uint8_t row, const char *text);
    void renderNoGpsDataScreen(const GpsSnapshot &snapshot);
    void renderWaitingFixScreen(const GpsSnapshot &snapshot);
    void renderFixScreen(const GpsSnapshot &snapshot);
};
