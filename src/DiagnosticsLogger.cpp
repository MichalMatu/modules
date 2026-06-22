#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"

namespace DiagnosticsLogger {

void printStartup()
{
    Serial.println();
    Serial.println("ESP32 CC1101 OOK/ASK sniffer");
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
    Serial.printf("CC1101 OOK: %.2f MHz br=%.1f kbps bw=%.1f kHz\n",
                  AppConfig::Cc1101FrequencyMhz,
                  AppConfig::Cc1101OokBitRateKbps,
                  AppConfig::Cc1101OokRxBandwidthKhz);
    Serial.printf("Serial Monitor: %lu\n",
                  static_cast<unsigned long>(AppConfig::SerialBaud));
    Serial.println("Mode: OOK/ASK receive-only pulse sniffer");
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

    Serial.printf("[diag] ook=ready listening=%s freq=%.2f rssi=%.1fdBm noise=%.1fdBm bursts=%lu decoded=%lu rejected=%lu overflow=%lu last_bits=%u repeat=%u hex=%s bits=%s\n",
                  snapshot.listening ? "yes" : "no",
                  snapshot.frequencyMhz,
                  snapshot.rssiDbm,
                  snapshot.noiseFloorDbm,
                  static_cast<unsigned long>(snapshot.burstCount),
                  static_cast<unsigned long>(snapshot.decodedBurstCount),
                  static_cast<unsigned long>(snapshot.rejectedBurstCount),
                  static_cast<unsigned long>(snapshot.edgeOverflowCount),
                  snapshot.lastBitCount,
                  snapshot.repeatCount,
                  snapshot.lastHex[0] != '\0' ? snapshot.lastHex : "-",
                  snapshot.lastBits);
}

} // namespace DiagnosticsLogger
