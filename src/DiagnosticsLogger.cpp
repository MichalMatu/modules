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
    Serial.println("ESP32 LOLIN32 OLED buttons MQ environment monitor");
    Serial.printf("Serial Monitor: %lu\n",
                  static_cast<unsigned long>(AppConfig::SerialBaud));
    Serial.println("Board: ESP32 LOLIN32 with LiPo charger");
    Serial.println("Power: OLED VCC=3V3 GND=GND; MQ modules need common GND");
    Serial.printf("OLED I2C: SDA=%u SCL=%u\n",
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin);
    Serial.printf("Buttons: B1=%u B2=%u B3=%u B4=%u active=%s\n",
                  AppConfig::Button1Pin,
                  AppConfig::Button2Pin,
                  AppConfig::Button3Pin,
                  AppConfig::Button4Pin,
                  AppConfig::ButtonPressedLevel == LOW ? "low" : "high");
    Serial.println("Buttons note: GPIO34/GPIO35 need external or module pull-ups");
    Serial.printf("MQ analog inputs: MQ-2 AO=GPIO%u, MQ-7 AO=GPIO%u, MQ-9 AO=GPIO%u\n",
                  AppConfig::Mq2AnalogPin,
                  AppConfig::Mq7AnalogPin,
                  AppConfig::Mq9AnalogPin);
    Serial.println("MQ note: keep ESP32 ADC inputs at or below 3.3V");
    Serial.println("Scheduler: FreeRTOS MQ polling, display, and diagnostics tasks");
}

void printHeartbeat(const MqSensors &sensors)
{
    const MqSensors::Snapshot data = sensors.snapshot();

    Serial.printf("[diag] uptime=%lums samples=%lu age=%lums mq2=%u/%u%% mq7=%u/%u%% mq9=%u/%u%% b1=%u b2=%u b3=%u b4=%u\n",
                  static_cast<unsigned long>(millis()),
                  static_cast<unsigned long>(data.sampleCount),
                  static_cast<unsigned long>(data.lastSampleAgeMs),
                  data.readings[MqSensors::Mq2].raw,
                  data.readings[MqSensors::Mq2].percent,
                  data.readings[MqSensors::Mq7].raw,
                  data.readings[MqSensors::Mq7].percent,
                  data.readings[MqSensors::Mq9].raw,
                  data.readings[MqSensors::Mq9].percent,
                  isButtonPressed(AppConfig::Button1Pin),
                  isButtonPressed(AppConfig::Button2Pin),
                  isButtonPressed(AppConfig::Button3Pin),
                  isButtonPressed(AppConfig::Button4Pin));
}

} // namespace DiagnosticsLogger
