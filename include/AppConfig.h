#pragma once

#include <Arduino.h>

namespace AppConfig {

inline constexpr uint8_t GpsRxPin = 26;     // ESP32 UART2 RX, connect to GPS TX.
inline constexpr uint8_t GpsTxPin = 25;     // ESP32 UART2 TX, connect to GPS RX.
inline constexpr uint32_t GpsBaudRates[] = {
    9600,
    38400,
    4800,
    19200,
    57600,
    115200,
};
inline constexpr size_t GpsBaudRateCount = sizeof(GpsBaudRates) / sizeof(GpsBaudRates[0]);
inline constexpr uint32_t GpsBaudProbeMs = 2200;
inline constexpr uint32_t GpsBaudRescanMs = 15000;
inline constexpr size_t GpsRxBufferSize = 2048;

inline constexpr uint8_t OledSdaPin = 5;
inline constexpr uint8_t OledSclPin = 4;

inline constexpr uint32_t SerialBaud = 115200;
inline constexpr bool SerialRawNmea = false;

inline constexpr uint32_t ScreenRefreshMs = 500;
inline constexpr uint32_t DiagnosticLogMs = 5000;
inline constexpr uint32_t BootScreenMs = 1500;
inline constexpr uint32_t NoGpsDataMs = 5000;
inline constexpr uint32_t FreshFixMaxAgeMs = 5000;

inline constexpr uint32_t GpsTaskDelayMs = 10;
inline constexpr uint32_t IdleTaskDelayMs = 1000;

inline constexpr uint32_t GpsTaskStack = 4096;
inline constexpr uint32_t DisplayTaskStack = 4096;
inline constexpr uint32_t DiagnosticsTaskStack = 4096;

inline constexpr UBaseType_t GpsTaskPriority = 3;
inline constexpr UBaseType_t DisplayTaskPriority = 2;
inline constexpr UBaseType_t DiagnosticsTaskPriority = 1;

inline constexpr BaseType_t GpsTaskCore = 1;
inline constexpr BaseType_t DisplayTaskCore = 1;
inline constexpr BaseType_t DiagnosticsTaskCore = 0;

} // namespace AppConfig
