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
    drawLine(0, "ESP32 CC1101");
    drawLine(1, "433 MHz RX");
    drawLine(2, "Receive only");
    drawLine(3, "SPI 18/19/23");
    drawLine(4, "CS27 GDO26/25");
    display_.sendBuffer();
}

void DisplayRenderer::renderRadioMissingScreen(const Cc1101Snapshot &snapshot)
{
    char line[24];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "CC1101 FAIL");
    snprintf(line, sizeof(line), "Code %d", snapshot.lastState);
    drawLine(1, line);
    drawLine(2, "Check SPI/power");
    snprintf(line, sizeof(line), "Last %lus",
             static_cast<unsigned long>(snapshot.lastInitAttemptAtMs / 1000));
    drawLine(3, line);
    drawLine(4, "Retrying...");
    display_.sendBuffer();
}

void DisplayRenderer::renderListeningScreen(const Cc1101Snapshot &snapshot)
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    snprintf(line, sizeof(line), "RX %.2f MHz", snapshot.frequencyMhz);
    drawLine(0, line);
    snprintf(line, sizeof(line), "State:%s",
             snapshot.listening ? "LISTEN" : "IDLE");
    drawLine(1, line);
    snprintf(line, sizeof(line), "Packets:%lu",
             static_cast<unsigned long>(snapshot.packetCount));
    drawLine(2, line);
    snprintf(line, sizeof(line), "RSSI %.1fdBm", snapshot.rssiDbm);
    drawLine(3, line);
    snprintf(line, sizeof(line), "CRC:%lu ERR:%lu",
             static_cast<unsigned long>(snapshot.crcErrorCount),
             static_cast<unsigned long>(snapshot.receiveErrorCount));
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderPacketScreen(const Cc1101Snapshot &snapshot)
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "PACKET RX");
    snprintf(line, sizeof(line), "Len:%u RSSI:%.1f",
             snapshot.lastPacketLength,
             snapshot.rssiDbm);
    drawLine(1, line);
    snprintf(line, sizeof(line), "LQI:%u Count:%lu",
             snapshot.lqi,
             static_cast<unsigned long>(snapshot.packetCount));
    drawLine(2, line);
    snprintf(line, sizeof(line), "Data %.16s", snapshot.lastPacketHex);
    drawLine(3, line);
    snprintf(line, sizeof(line), "%.16s", snapshot.lastPacketHex + 16);
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::render(const Cc1101Snapshot &snapshot)
{
    if (millis() - startedAtMs_ < AppConfig::BootScreenMs) {
        renderBootScreen();
        return;
    }

    if (!snapshot.radioReady) {
        renderRadioMissingScreen(snapshot);
        return;
    }

    if (snapshot.recentPacket) {
        renderPacketScreen(snapshot);
        return;
    }

    renderListeningScreen(snapshot);
}
