#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "Cc1101Snapshot.h"

class DisplayRenderer {
public:
    DisplayRenderer();

    void begin();
    void render(const Cc1101Snapshot &snapshot);

private:
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_;
    uint32_t startedAtMs_ = 0;

    void drawLine(uint8_t row, const char *text);
    void renderBootScreen();
    void renderRadioMissingScreen(const Cc1101Snapshot &snapshot);
    void renderListeningScreen(const Cc1101Snapshot &snapshot);
    void renderPacketScreen(const Cc1101Snapshot &snapshot);
};
