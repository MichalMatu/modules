#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

#include "MqSensors.h"

class DisplayRenderer {
public:
    explicit DisplayRenderer(MqSensors &sensors);

    void begin();
    void render();

private:
    enum class Screen : uint8_t {
        Main,
        Wiring,
        Details,
    };

    MqSensors &sensors_;
    U8G2_SSD1306_128X64_NONAME_F_HW_I2C display_;
    uint32_t startedAtMs_ = 0;
    Screen screen_ = Screen::Main;
    bool showRaw_ = false;
    bool lastButton1Pressed_ = false;
    bool lastButton2Pressed_ = false;
    bool lastButton3Pressed_ = false;
    bool lastButton4Pressed_ = false;

    void drawLine(uint8_t row, const char *text);
    void handleButtons();
    void selectPreviousScreen();
    void selectNextScreen();
    void renderBootScreen();
    void renderMainScreen();
    void renderWiringScreen();
    void renderDetailsScreen();
};
