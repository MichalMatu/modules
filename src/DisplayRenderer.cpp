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
    drawLine(1, "433 OOK/ASK");
    drawLine(2, "Receive only");
    drawLine(3, "Press remote");
    drawLine(4, "GDO0 GPIO26");
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
    snprintf(line, sizeof(line), "OOK %.2f MHz", snapshot.frequencyMhz);
    drawLine(0, line);
    snprintf(line, sizeof(line), "%s RSSI %.0f",
             snapshot.listening ? "ON" : "IDLE",
             snapshot.rssiDbm);
    drawLine(1, line);
    snprintf(line, sizeof(line), "Noise %.0fdBm", snapshot.noiseFloorDbm);
    drawLine(2, line);
    snprintf(line, sizeof(line), "Bursts:%lu Dec:%lu",
             static_cast<unsigned long>(snapshot.burstCount),
             static_cast<unsigned long>(snapshot.decodedBurstCount));
    drawLine(3, line);
    snprintf(line, sizeof(line), "Reject:%lu Ovf:%lu",
             static_cast<unsigned long>(snapshot.rejectedBurstCount),
             static_cast<unsigned long>(snapshot.edgeOverflowCount));
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderBurstScreen(const Cc1101Snapshot &snapshot)
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, snapshot.lastDecoded ? "OOK CODE" : "OOK BURST");
    snprintf(line, sizeof(line), "Bits:%u Rep:%u",
             snapshot.lastBitCount,
             snapshot.repeatCount);
    drawLine(1, line);
    if (snapshot.lastHex[0] != '\0') {
        snprintf(line, sizeof(line), "%.21s", snapshot.lastHex);
    } else {
        snprintf(line, sizeof(line), "%.21s", snapshot.lastBits);
    }
    drawLine(2, line);
    snprintf(line, sizeof(line), "U:%uus P:%u",
             snapshot.lastUnitUs,
             snapshot.lastPulseCount);
    drawLine(3, line);
    snprintf(line, sizeof(line), "RSSI %.0fdBm%s",
             snapshot.lastBurstRssiDbm,
             snapshot.lastInverted ? " INV" : "");
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

    if (snapshot.recentBurst) {
        renderBurstScreen(snapshot);
        return;
    }

    renderListeningScreen(snapshot);
}
