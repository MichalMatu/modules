#include "DisplayRenderer.h"

#include <Wire.h>

#include "AppConfig.h"

namespace {

bool isButtonPressed(uint8_t pin)
{
    return digitalRead(pin) == AppConfig::ButtonPressedLevel;
}

bool pressedEdge(bool pressed, bool lastPressed)
{
    return pressed && !lastPressed;
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

DisplayRenderer::DisplayRenderer(MqSensors &sensors)
    : sensors_(sensors),
      display_(U8G2_R0,
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

void DisplayRenderer::handleButtons()
{
    const bool button1Pressed = isButtonPressed(AppConfig::Button1Pin);
    const bool button2Pressed = isButtonPressed(AppConfig::Button2Pin);
    const bool button3Pressed = isButtonPressed(AppConfig::Button3Pin);
    const bool button4Pressed = isButtonPressed(AppConfig::Button4Pin);

    if (pressedEdge(button1Pressed, lastButton1Pressed_)) {
        selectPreviousScreen();
    }
    if (pressedEdge(button2Pressed, lastButton2Pressed_)) {
        selectNextScreen();
    }
    if (pressedEdge(button3Pressed, lastButton3Pressed_)) {
        sensors_.resetStats();
    }
    if (pressedEdge(button4Pressed, lastButton4Pressed_)) {
        showRaw_ = !showRaw_;
    }

    lastButton1Pressed_ = button1Pressed;
    lastButton2Pressed_ = button2Pressed;
    lastButton3Pressed_ = button3Pressed;
    lastButton4Pressed_ = button4Pressed;
}

void DisplayRenderer::selectPreviousScreen()
{
    switch (screen_) {
    case Screen::Main:
        screen_ = Screen::Details;
        break;
    case Screen::Wiring:
        screen_ = Screen::Main;
        break;
    case Screen::Details:
        screen_ = Screen::Wiring;
        break;
    }
}

void DisplayRenderer::selectNextScreen()
{
    switch (screen_) {
    case Screen::Main:
        screen_ = Screen::Wiring;
        break;
    case Screen::Wiring:
        screen_ = Screen::Details;
        break;
    case Screen::Details:
        screen_ = Screen::Main;
        break;
    }
}

void DisplayRenderer::renderBootScreen()
{
    char line[24];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "ENV MONITOR");
    snprintf(line, sizeof(line), "MQ2  GPIO%u", AppConfig::Mq2AnalogPin);
    drawLine(1, line);
    snprintf(line, sizeof(line), "MQ7  GPIO%u", AppConfig::Mq7AnalogPin);
    drawLine(2, line);
    snprintf(line, sizeof(line), "MQ9  GPIO%u", AppConfig::Mq9AnalogPin);
    drawLine(3, line);
    snprintf(line, sizeof(line), "OLED SDA%u SCL%u", AppConfig::OledSdaPin, AppConfig::OledSclPin);
    drawLine(4, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderMainScreen()
{
    char line[32];
    const MqSensors::Snapshot data = sensors_.snapshot();

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, showRaw_ ? "ENV MONITOR RAW" : "ENV MONITOR %");

    for (uint8_t i = 0; i < MqSensors::SensorCount; ++i) {
        const MqSensors::Reading &reading = data.readings[i];
        if (showRaw_) {
            snprintf(line, sizeof(line), "%s raw %u", reading.name, reading.raw);
        } else {
            snprintf(line, sizeof(line), "%s %u%% raw %u", reading.name, reading.percent, reading.raw);
        }
        drawLine(1 + i, line);
    }

    snprintf(line, sizeof(line), "Samples %lu", static_cast<unsigned long>(data.sampleCount));
    drawLine(4, line);
    drawLine(5, "B3 reset B4 raw/%");
    display_.sendBuffer();
}

void DisplayRenderer::renderWiringScreen()
{
    char line[32];

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "WIRING");
    snprintf(line, sizeof(line), "MQ2 AO -> GPIO%u", AppConfig::Mq2AnalogPin);
    drawLine(1, line);
    snprintf(line, sizeof(line), "MQ7 AO -> GPIO%u", AppConfig::Mq7AnalogPin);
    drawLine(2, line);
    snprintf(line, sizeof(line), "MQ9 AO -> GPIO%u", AppConfig::Mq9AnalogPin);
    drawLine(3, line);
    drawLine(4, "ADC max 3.3V");
    snprintf(line, sizeof(line), "OLED %u/%u", AppConfig::OledSdaPin, AppConfig::OledSclPin);
    drawLine(5, line);
    display_.sendBuffer();
}

void DisplayRenderer::renderDetailsScreen()
{
    char line[32];
    const MqSensors::Snapshot data = sensors_.snapshot();

    display_.clearBuffer();
    display_.setFont(u8g2_font_5x8_tf);
    drawLine(0, "MQ MIN/MAX RAW");
    for (uint8_t i = 0; i < MqSensors::SensorCount; ++i) {
        const MqSensors::Reading &reading = data.readings[i];
        snprintf(line,
                 sizeof(line),
                 "%s %u/%u now %u",
                 reading.name,
                 reading.minRaw,
                 reading.maxRaw,
                 reading.raw);
        drawLine(1 + i, line);
    }
    snprintf(line, sizeof(line), "Last %lums", static_cast<unsigned long>(data.lastSampleAgeMs));
    drawLine(4, line);
    drawLine(5, "B1/B2 screens");
    display_.sendBuffer();
}

void DisplayRenderer::render()
{
    if (millis() - startedAtMs_ < AppConfig::BootScreenMs) {
        renderBootScreen();
        return;
    }

    handleButtons();

    switch (screen_) {
    case Screen::Main:
        renderMainScreen();
        break;
    case Screen::Wiring:
        renderWiringScreen();
        break;
    case Screen::Details:
        renderDetailsScreen();
        break;
    }
}
