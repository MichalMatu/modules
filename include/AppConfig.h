#pragma once

#include <Arduino.h>

namespace AppConfig {

inline constexpr uint8_t OledSdaPin = 5;
inline constexpr uint8_t OledSclPin = 4;
inline constexpr uint8_t Max30100Address = 0x57;
inline constexpr uint32_t I2cClockHz = 400000;

inline constexpr uint32_t SerialBaud = 115200;

inline constexpr uint32_t ScreenRefreshMs = 500;
inline constexpr uint32_t DiagnosticLogMs = 2000;
inline constexpr uint32_t BootScreenMs = 1500;
inline constexpr uint32_t Max30100TaskDelayMs = 5;
inline constexpr uint32_t Max30100PublishMs = 250;
inline constexpr uint32_t Max30100RetryMs = 5000;
inline constexpr uint32_t BeatIndicatorMs = 250;

inline constexpr uint32_t IdleTaskDelayMs = 1000;

inline constexpr uint32_t Max30100TaskStack = 4096;
inline constexpr uint32_t DisplayTaskStack = 4096;
inline constexpr uint32_t DiagnosticsTaskStack = 4096;

inline constexpr UBaseType_t Max30100TaskPriority = 3;
inline constexpr UBaseType_t DisplayTaskPriority = 2;
inline constexpr UBaseType_t DiagnosticsTaskPriority = 1;

inline constexpr BaseType_t Max30100TaskCore = 1;
inline constexpr BaseType_t DisplayTaskCore = 1;
inline constexpr BaseType_t DiagnosticsTaskCore = 0;

} // namespace AppConfig
