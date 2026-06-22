#pragma once

#include <Arduino.h>

namespace AppConfig {

inline constexpr uint8_t OledSdaPin = 5;
inline constexpr uint8_t OledSclPin = 4;

inline constexpr uint8_t Cc1101SckPin = 18;
inline constexpr uint8_t Cc1101MisoPin = 19;
inline constexpr uint8_t Cc1101MosiPin = 23;
inline constexpr uint8_t Cc1101CsnPin = 27;
inline constexpr uint8_t Cc1101Gdo0Pin = 26;
inline constexpr uint8_t Cc1101Gdo2Pin = 25;

inline constexpr float Cc1101FrequencyMhz = 433.92f;
inline constexpr float Cc1101BitRateKbps = 4.8f;
inline constexpr float Cc1101FrequencyDeviationKhz = 5.0f;
inline constexpr float Cc1101RxBandwidthKhz = 125.0f;
inline constexpr int8_t Cc1101OutputPowerDbm = 0;
inline constexpr uint16_t Cc1101PreambleLengthBits = 16;

inline constexpr uint32_t SerialBaud = 115200;

inline constexpr uint32_t ScreenRefreshMs = 500;
inline constexpr uint32_t DiagnosticLogMs = 2000;
inline constexpr uint32_t BootScreenMs = 1500;
inline constexpr uint32_t Cc1101TaskDelayMs = 20;
inline constexpr uint32_t Cc1101RetryMs = 5000;

inline constexpr uint32_t IdleTaskDelayMs = 1000;

inline constexpr uint32_t Cc1101TaskStack = 4096;
inline constexpr uint32_t DisplayTaskStack = 4096;
inline constexpr uint32_t DiagnosticsTaskStack = 4096;

inline constexpr UBaseType_t Cc1101TaskPriority = 3;
inline constexpr UBaseType_t DisplayTaskPriority = 2;
inline constexpr UBaseType_t DiagnosticsTaskPriority = 1;

inline constexpr BaseType_t Cc1101TaskCore = 1;
inline constexpr BaseType_t DisplayTaskCore = 1;
inline constexpr BaseType_t DiagnosticsTaskCore = 0;

} // namespace AppConfig
