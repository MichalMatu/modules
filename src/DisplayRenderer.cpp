#include "DisplayRenderer.h"

#include <Wire.h>

#include "AppConfig.h"
#include "AppLog.h"

DisplayRenderer::DisplayRenderer()
    : display_(HardwareProfile::Oled.rotation,
               U8X8_PIN_NONE,
               HardwareProfile::Oled.sclPin,
               HardwareProfile::Oled.sdaPin)
{
}

void DisplayRenderer::begin()
{
    Wire.begin(HardwareProfile::Oled.sdaPin, HardwareProfile::Oled.sclPin);
    display_.begin();
    startedAtMs_ = millis();
    renderBootScreen();
    APP_LOGI("display", "OLED initialized on SDA=%u SCL=%u",
             HardwareProfile::Oled.sdaPin,
             HardwareProfile::Oled.sclPin);
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
    drawLine(0, HardwareProfile::ProjectName);
    drawLine(1, "Base firmware");
    drawLine(2, "No module code");
    snprintf(line, sizeof(line), "SDA%u SCL%u",
             HardwareProfile::Oled.sdaPin,
             HardwareProfile::Oled.sclPin);
    drawLine(3, line);
    snprintf(line, sizeof(line), "Serial %lu",
             static_cast<unsigned long>(AppConfig::SerialBaud));
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderStatusScreen()
{
    char line[24];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "BASE READY");
    snprintf(line, sizeof(line), "Uptime %lus",
             static_cast<unsigned long>(millis() / 1000));
    drawLine(1, line);
    snprintf(line, sizeof(line), "OLED SDA%u SCL%u",
             HardwareProfile::Oled.sdaPin,
             HardwareProfile::Oled.sclPin);
    drawLine(2, line);
    drawLine(3, "Module variants");
    drawLine(4, "use branches");
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
