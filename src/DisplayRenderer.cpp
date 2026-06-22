#include "DisplayRenderer.h"

#include <Wire.h>

#include "AppConfig.h"

DisplayRenderer::DisplayRenderer()
    : display_(U8G2_R2,
               U8X8_PIN_NONE,
               AppConfig::OledSclPin,
               AppConfig::OledSdaPin)
{
}

void DisplayRenderer::begin()
{
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
    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "ESP32 OLED GPS");
    drawLine(1, "u-blox NEO-M8M");
    drawLine(2, "UART2 RX26 TX25");
    drawLine(3, "OLED SDA5 SCL4");
    drawLine(4, "GPS auto baud");
    display_.sendBuffer();
}

void DisplayRenderer::renderNoGpsDataScreen(const GpsSnapshot &snapshot)
{
    char line[24];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "NO GPS DATA");
    drawLine(1, "Check RX/TX");
    drawLine(2, "Auto baud scan");
    snprintf(line, sizeof(line), "Baud: %lu", snapshot.activeBaud);
    drawLine(3, line);
    snprintf(line, sizeof(line), "Chars: %lu", snapshot.charsProcessed);
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderWaitingFixScreen(const GpsSnapshot &snapshot)
{
    char line[28];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    snprintf(line, sizeof(line), "WAIT FIX S:%lu", snapshot.satellites);
    drawLine(0, line);
    snprintf(line, sizeof(line), "Chars:%lu", snapshot.charsProcessed);
    drawLine(1, line);
    snprintf(line, sizeof(line), "OK:%lu BAD:%lu",
             snapshot.passedChecksum,
             snapshot.failedChecksum);
    drawLine(2, line);
    snprintf(line, sizeof(line), "HDOP:%.1f", snapshot.hdop);
    drawLine(3, line);
    snprintf(line, sizeof(line), "Baud:%lu", snapshot.activeBaud);
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderFixScreen(const GpsSnapshot &snapshot)
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);

    snprintf(line, sizeof(line), "FIX S:%lu H:%.1f",
             snapshot.satellites,
             snapshot.hdop);
    drawLine(0, line);

    snprintf(line, sizeof(line), "Lat %.6f", snapshot.latitude);
    drawLine(1, line);

    snprintf(line, sizeof(line), "Lon %.6f", snapshot.longitude);
    drawLine(2, line);

    snprintf(line, sizeof(line), "V%.1f A%.0fm",
             snapshot.speedKmph,
             snapshot.altitudeMeters);
    drawLine(3, line);

    if (snapshot.timeValid && snapshot.dateValid) {
        snprintf(line, sizeof(line), "%02u:%02u:%02u %02u.%02u.%02u",
                 snapshot.hour,
                 snapshot.minute,
                 snapshot.second,
                 snapshot.day,
                 snapshot.month,
                 snapshot.year % 100);
    } else if (snapshot.timeValid) {
        snprintf(line, sizeof(line), "%02u:%02u:%02u --.--.--",
                 snapshot.hour,
                 snapshot.minute,
                 snapshot.second);
    } else {
        snprintf(line, sizeof(line), "--:--:-- --.--.--");
    }
    drawLine(4, line);

    display_.sendBuffer();
}

void DisplayRenderer::render(const GpsSnapshot &snapshot)
{
    const uint32_t now = millis();

    if (now - startedAtMs_ < AppConfig::BootScreenMs) {
        renderBootScreen();
        return;
    }

    if (!snapshot.dataReceived && now - startedAtMs_ > AppConfig::NoGpsDataMs) {
        renderNoGpsDataScreen(snapshot);
        return;
    }

    if (!snapshot.freshFix) {
        renderWaitingFixScreen(snapshot);
        return;
    }

    renderFixScreen(snapshot);
}
