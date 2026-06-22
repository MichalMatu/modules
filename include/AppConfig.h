#pragma once

#include <Arduino.h>

namespace AppConfig {

inline constexpr uint8_t OledSdaPin = 25;
inline constexpr uint8_t OledSclPin = 26;

inline constexpr uint8_t Button1Pin = 34;
inline constexpr uint8_t Button2Pin = 35;
inline constexpr uint8_t Button3Pin = 32;
inline constexpr uint8_t Button4Pin = 33;
inline constexpr uint8_t ButtonPressedLevel = LOW;

inline constexpr uint32_t SerialBaud = 115200;

inline constexpr uint32_t ScreenRefreshMs = 250;
inline constexpr uint32_t DiagnosticLogMs = 5000;
inline constexpr uint32_t BootScreenMs = 1500;
inline constexpr uint32_t IdleTaskDelayMs = 1000;

inline constexpr uint32_t DisplayTaskStack = 4096;
inline constexpr uint32_t DiagnosticsTaskStack = 4096;

inline constexpr UBaseType_t DisplayTaskPriority = 2;
inline constexpr UBaseType_t DiagnosticsTaskPriority = 1;

inline constexpr BaseType_t DisplayTaskCore = 1;
inline constexpr BaseType_t DiagnosticsTaskCore = 0;

} // namespace AppConfig
