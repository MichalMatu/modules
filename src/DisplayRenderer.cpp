#include "DisplayRenderer.h"

#include "AppConfig.h"
#include "I2cBus.h"

namespace {

constexpr float DisplaySmoothingAlpha = 0.25f;
constexpr uint32_t BeatsForFullSignalProgress = 5;

} // namespace

DisplayRenderer::DisplayRenderer()
    : display_(U8G2_R2,
               U8X8_PIN_NONE,
               AppConfig::OledSclPin,
               AppConfig::OledSdaPin)
{
}

void DisplayRenderer::begin()
{
    I2cBus::begin();
    I2cBus::lock();
    display_.begin();
    I2cBus::unlock();
    startedAtMs_ = millis();
    renderBootScreen();
}

void DisplayRenderer::drawLine(uint8_t row, const char *text)
{
    display_.drawStr(0, 8 + row * 10, text);
}

void DisplayRenderer::drawFingerPromptIcon()
{
    display_.drawRFrame(49, 1, 30, 48, 12);
    display_.drawRFrame(55, 7, 18, 36, 8);
    display_.drawVLine(64, 42, 8);

    display_.drawRFrame(28, 49, 72, 14, 4);
    display_.drawHLine(42, 55, 44);
    display_.drawHLine(48, 59, 32);

    display_.drawLine(25, 12, 38, 25);
    display_.drawLine(38, 25, 31, 25);
    display_.drawLine(38, 25, 38, 18);

    display_.drawLine(103, 12, 90, 25);
    display_.drawLine(90, 25, 97, 25);
    display_.drawLine(90, 25, 90, 18);

    display_.drawLine(16, 31, 28, 41);
    display_.drawLine(28, 41, 22, 41);
    display_.drawLine(28, 41, 28, 35);

    display_.drawLine(112, 31, 100, 41);
    display_.drawLine(100, 41, 106, 41);
    display_.drawLine(100, 41, 100, 35);
}

void DisplayRenderer::drawSignalProgress(uint32_t beatCount)
{
    const uint32_t cappedBeatCount = beatCount > BeatsForFullSignalProgress
                                         ? BeatsForFullSignalProgress
                                         : beatCount;
    const uint8_t width = static_cast<uint8_t>((68 * cappedBeatCount) /
                                               BeatsForFullSignalProgress);

    display_.drawRFrame(29, 50, 70, 12, 3);
    if (width > 0) {
        display_.drawBox(31, 52, width, 8);
    }
}

void DisplayRenderer::updateSmoothedValues(const Max30100Snapshot &snapshot)
{
    if (snapshot.heartRateValid) {
        if (!hasSmoothedHeartRate_) {
            smoothedHeartRateBpm_ = snapshot.heartRateBpm;
            hasSmoothedHeartRate_ = true;
        } else {
            smoothedHeartRateBpm_ +=
                (snapshot.heartRateBpm - smoothedHeartRateBpm_) * DisplaySmoothingAlpha;
        }
    }

    if (snapshot.spo2Valid) {
        if (!hasSmoothedSpo2_) {
            smoothedSpo2_ = static_cast<float>(snapshot.spo2);
            hasSmoothedSpo2_ = true;
        } else {
            smoothedSpo2_ +=
                (static_cast<float>(snapshot.spo2) - smoothedSpo2_) * DisplaySmoothingAlpha;
        }
    }
}

void DisplayRenderer::renderBootScreen()
{
    I2cBus::lock();
    display_.clearBuffer();
    drawFingerPromptIcon();
    display_.sendBuffer();
    I2cBus::unlock();
}

void DisplayRenderer::renderSensorMissingScreen(const Max30100Snapshot &snapshot)
{
    char line[24];

    I2cBus::lock();
    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "MAX30100 FAIL");
    drawLine(1, "Check I2C/power");
    snprintf(line, sizeof(line), "Addr 0x%02X",
             AppConfig::Max30100Address);
    drawLine(2, line);
    snprintf(line, sizeof(line), "Last try %lus",
             static_cast<unsigned long>(snapshot.lastInitAttemptAtMs / 1000));
    drawLine(3, line);
    drawLine(4, "Retrying...");
    display_.sendBuffer();
    I2cBus::unlock();
}

void DisplayRenderer::renderWaitingFingerScreen(const Max30100Snapshot &snapshot)
{
    (void)snapshot;
    I2cBus::lock();
    display_.clearBuffer();
    drawFingerPromptIcon();
    display_.sendBuffer();
    I2cBus::unlock();
}

void DisplayRenderer::renderAcquiringSignalScreen(const Max30100Snapshot &snapshot)
{
    I2cBus::lock();
    display_.clearBuffer();
    drawFingerPromptIcon();
    drawSignalProgress(snapshot.beatCount);
    display_.sendBuffer();
    I2cBus::unlock();
}

void DisplayRenderer::renderMeasurementScreen(const Max30100Snapshot &snapshot)
{
    char bpmValue[8];
    char spo2Value[8];
    char statusLine[32];

    updateSmoothedValues(snapshot);

    if (hasSmoothedHeartRate_ && snapshot.heartRateValid) {
        const uint16_t bpm = static_cast<uint16_t>(smoothedHeartRateBpm_ + 0.5f);
        snprintf(bpmValue, sizeof(bpmValue), "%u", bpm);
    } else {
        snprintf(bpmValue, sizeof(bpmValue), "--");
    }

    if (hasSmoothedSpo2_ && snapshot.spo2Valid) {
        const uint8_t spo2 = static_cast<uint8_t>(smoothedSpo2_ + 0.5f);
        snprintf(spo2Value, sizeof(spo2Value), "%u", spo2);
    } else {
        snprintf(spo2Value, sizeof(spo2Value), "--");
    }

    I2cBus::lock();
    display_.clearBuffer();

    display_.setFont(u8g2_font_5x8_tf);
    display_.drawStr(0, 8, "bpm:");
    display_.drawStr(72, 8, "spo2:");
    display_.drawVLine(63, 0, 46);

    display_.setFont(u8g2_font_logisoso22_tf);
    display_.drawStr(0, 40, bpmValue);
    display_.drawStr(72, 40, spo2Value);

    display_.setFont(u8g2_font_5x8_tf);
    display_.drawHLine(0, 51, 128);
    snprintf(statusLine, sizeof(statusLine), "beat:%s cnt:%lu led:%u",
             snapshot.freshBeat ? "*" : "-",
             static_cast<unsigned long>(snapshot.beatCount),
             snapshot.redLedCurrentBias);
    display_.drawStr(0, 63, statusLine);
    display_.sendBuffer();
    I2cBus::unlock();
}

void DisplayRenderer::render(const Max30100Snapshot &snapshot)
{
    if (millis() - startedAtMs_ < AppConfig::BootScreenMs) {
        renderBootScreen();
        return;
    }

    if (!snapshot.sensorFound) {
        renderSensorMissingScreen(snapshot);
        return;
    }

    if (!snapshot.heartRateValid && !snapshot.spo2Valid) {
        if (snapshot.beatCount > 0) {
            renderAcquiringSignalScreen(snapshot);
            return;
        }

        renderWaitingFingerScreen(snapshot);
        return;
    }

    renderMeasurementScreen(snapshot);
}
