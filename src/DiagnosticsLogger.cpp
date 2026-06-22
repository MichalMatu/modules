#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"

namespace DiagnosticsLogger {

namespace {

bool isButtonPressed(uint8_t pin)
{
    return digitalRead(pin) == AppConfig::ButtonPressedLevel;
}

} // namespace

void printStartup()
{
    Serial.println();
    Serial.println("ESP32 LOLIN32 OLED buttons firmware");
    Serial.printf("Serial Monitor: %lu\n",
                  static_cast<unsigned long>(AppConfig::SerialBaud));
    Serial.println("Board: ESP32 LOLIN32 with LiPo charger");
    Serial.printf("OLED I2C: SDA=%u SCL=%u\n",
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin);
    Serial.printf("Buttons: B1=%u B2=%u B3=%u B4=%u active=%s\n",
                  AppConfig::Button1Pin,
                  AppConfig::Button2Pin,
                  AppConfig::Button3Pin,
                  AppConfig::Button4Pin,
                  AppConfig::ButtonPressedLevel == LOW ? "low" : "high");
    Serial.println("Scheduler: FreeRTOS display and diagnostics tasks");
}

void printHeartbeat()
{
    Serial.printf("[diag] uptime=%lums board=lolin32-battery oled=sda%u/scl%u b1=%u b2=%u b3=%u b4=%u\n",
                  static_cast<unsigned long>(millis()),
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin,
                  isButtonPressed(AppConfig::Button1Pin),
                  isButtonPressed(AppConfig::Button2Pin),
                  isButtonPressed(AppConfig::Button3Pin),
                  isButtonPressed(AppConfig::Button4Pin));
}

} // namespace DiagnosticsLogger
