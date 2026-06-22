#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"

namespace DiagnosticsLogger {

void printStartup()
{
    Serial.println();
    Serial.println("ESP32 CC1101 receiver");
    Serial.printf("OLED I2C: SDA=%u SCL=%u\n",
                  AppConfig::OledSdaPin,
                  AppConfig::OledSclPin);
    Serial.printf("CC1101 SPI: SCK=%u MISO=%u MOSI=%u CSN=%u GDO0=%u GDO2=%u\n",
                  AppConfig::Cc1101SckPin,
                  AppConfig::Cc1101MisoPin,
                  AppConfig::Cc1101MosiPin,
                  AppConfig::Cc1101CsnPin,
                  AppConfig::Cc1101Gdo0Pin,
                  AppConfig::Cc1101Gdo2Pin);
    Serial.printf("CC1101 RX: %.2f MHz br=%.1f kbps dev=%.1f kHz bw=%.1f kHz\n",
                  AppConfig::Cc1101FrequencyMhz,
                  AppConfig::Cc1101BitRateKbps,
                  AppConfig::Cc1101FrequencyDeviationKhz,
                  AppConfig::Cc1101RxBandwidthKhz);
    Serial.printf("Serial Monitor: %lu\n",
                  static_cast<unsigned long>(AppConfig::SerialBaud));
    Serial.println("Mode: receive only");
}

void printSnapshot(const Cc1101Snapshot &snapshot)
{
    if (!snapshot.radioReady) {
        Serial.printf("[diag] cc1101=missing state=%d uptime=%lums last_try=%lums\n",
                      snapshot.lastState,
                      static_cast<unsigned long>(snapshot.uptimeMs),
                      static_cast<unsigned long>(snapshot.lastInitAttemptAtMs));
        return;
    }

    Serial.printf("[diag] cc1101=ready listening=%s freq=%.2f packets=%lu crc=%lu err=%lu rssi=%.1fdBm lqi=%u len=%u data=%s\n",
                  snapshot.listening ? "yes" : "no",
                  snapshot.frequencyMhz,
                  static_cast<unsigned long>(snapshot.packetCount),
                  static_cast<unsigned long>(snapshot.crcErrorCount),
                  static_cast<unsigned long>(snapshot.receiveErrorCount),
                  snapshot.rssiDbm,
                  snapshot.lqi,
                  snapshot.lastPacketLength,
                  snapshot.lastPacketHex);
}

} // namespace DiagnosticsLogger
