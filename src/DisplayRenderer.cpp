#include "DisplayRenderer.h"

#include <Wire.h>

#include "AppConfig.h"

namespace {

bool isButtonPressed(uint8_t pin)
{
    return digitalRead(pin) == AppConfig::ButtonPressedLevel;
}

void setupButtonPin(uint8_t pin)
{
    if (pin == 34 || pin == 35 || pin == 36 || pin == 39) {
        pinMode(pin, INPUT);
        return;
    }

    pinMode(pin, INPUT_PULLUP);
}

} // namespace

DisplayRenderer::DisplayRenderer()
    : display_(U8G2_R0,
               U8X8_PIN_NONE,
               AppConfig::OledSclPin,
               AppConfig::OledSdaPin)
{
}

void DisplayRenderer::begin()
{
    setupButtonPin(AppConfig::Button1Pin);
    setupButtonPin(AppConfig::Button2Pin);
    setupButtonPin(AppConfig::Button3Pin);
    setupButtonPin(AppConfig::Button4Pin);

    Wire.begin(AppConfig::OledSdaPin, AppConfig::OledSclPin);
    display_.begin();
    startedAtMs_ = millis();
    renderBootScreen();
}

void DisplayRenderer::drawLine(uint8_t row, const char *text)
{
    display_.drawStr(0, 8 + row * 10, text);
}

void DisplayRenderer::renderBootScreen()
{
    char line[24];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "LOLIN32 OLED BTN");
    snprintf(line, sizeof(line), "SDA%u SCL%u", AppConfig::OledSdaPin, AppConfig::OledSclPin);
    drawLine(1, line);
    snprintf(line, sizeof(line), "B1 %u B2 %u", AppConfig::Button1Pin, AppConfig::Button2Pin);
    drawLine(2, line);
    snprintf(line, sizeof(line), "B3 %u B4 %u", AppConfig::Button3Pin, AppConfig::Button4Pin);
    drawLine(3, line);
    drawLine(4, "Serial 115200");
    display_.sendBuffer();
}

void DisplayRenderer::renderStatusScreen()
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "OLED BUTTON TEST");
    snprintf(line, sizeof(line), "Uptime %lus", static_cast<unsigned long>(millis() / 1000));
    drawLine(1, line);
    snprintf(line,
             sizeof(line),
             "B1:%c B2:%c",
             isButtonPressed(AppConfig::Button1Pin) ? '*' : '-',
             isButtonPressed(AppConfig::Button2Pin) ? '*' : '-');
    drawLine(2, line);
    snprintf(line,
             sizeof(line),
             "B3:%c B4:%c",
             isButtonPressed(AppConfig::Button3Pin) ? '*' : '-',
             isButtonPressed(AppConfig::Button4Pin) ? '*' : '-');
    drawLine(3, line);
    snprintf(line, sizeof(line), "I2C %u/%u", AppConfig::OledSdaPin, AppConfig::OledSclPin);
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::render()
{
    if (millis() - startedAtMs_ < AppConfig::BootScreenMs) {
        renderBootScreen();
        return;
    }

    renderStatusScreen();
}
