#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"

namespace DiagnosticsLogger {

void printStartup()
{
    Serial.println();
    Serial.println("ESP32 MAX30100 pulse oximeter");
    Serial.printf("OLED I2C: SDA=%u SCL=%u\n",
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin);
    Serial.printf("MAX30100 I2C: address=0x%02X shared-bus\n",
                  AppConfig::Max30100Address);
    Serial.printf("Serial Monitor: %lu\n",
                  static_cast<unsigned long>(AppConfig::SerialBaud));
    Serial.println("Scheduler: MAX30100 polling, OLED rendering, diagnostics");
}

void printSnapshot(const Max30100Snapshot &snapshot)
{
    if (!snapshot.sensorFound) {
        Serial.printf("[diag] max30100=missing addr=0x%02X uptime=%lums last_try=%lums\n",
                      AppConfig::Max30100Address,
                      static_cast<unsigned long>(snapshot.uptimeMs),
                      static_cast<unsigned long>(snapshot.lastInitAttemptAtMs));
        return;
    }

    Serial.printf(
        "[diag] bpm=%.1f bpm_valid=%s spo2=%u spo2_valid=%s beat=%s count=%lu led_bias=%u uptime=%lums\n",
        snapshot.heartRateBpm,
        snapshot.heartRateValid ? "yes" : "no",
        snapshot.spo2,
        snapshot.spo2Valid ? "yes" : "no",
        snapshot.freshBeat ? "yes" : "no",
        static_cast<unsigned long>(snapshot.beatCount),
        snapshot.redLedCurrentBias,
        static_cast<unsigned long>(snapshot.uptimeMs));
}

} // namespace DiagnosticsLogger
