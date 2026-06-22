#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"

namespace DiagnosticsLogger {

void printStartup()
{
    Serial.println();
    Serial.println("ESP32 OLED GPS logger");
    Serial.printf("GPS UART2: RX=%u TX=%u auto-baud\n",
                  AppConfig::GpsRxPin,
                  AppConfig::GpsTxPin);
    Serial.print("GPS baud candidates:");
    for (size_t i = 0; i < AppConfig::GpsBaudRateCount; ++i) {
        Serial.printf(" %lu", AppConfig::GpsBaudRates[i]);
    }
    Serial.println();
    Serial.printf("OLED I2C: SDA=%u SCL=%u\n",
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin);
    Serial.println("Scheduler: FreeRTOS periodic tasks");
}

void printSnapshot(const GpsSnapshot &snapshot)
{
    Serial.printf(
        "[diag] baud=%lu detected=%s chars=%lu ok=%lu bad=%lu fix=%s sats=%lu hdop=%.1f age=%lums\n",
        snapshot.activeBaud,
        snapshot.baudDetected ? "yes" : "no",
        snapshot.charsProcessed,
        snapshot.passedChecksum,
        snapshot.failedChecksum,
        snapshot.freshFix ? "yes" : "no",
        snapshot.satellites,
        snapshot.hdop,
        snapshot.locationAgeMs);

    if (snapshot.freshFix) {
        Serial.printf(
            "[diag] lat=%.6f lon=%.6f speed=%.1fkm/h alt=%.1fm\n",
            snapshot.latitude,
            snapshot.longitude,
            snapshot.speedKmph,
            snapshot.altitudeMeters);
    }
}

} // namespace DiagnosticsLogger
