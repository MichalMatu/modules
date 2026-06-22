#pragma once

#include <Arduino.h>
#include <U8g2lib.h>

namespace HardwareProfile {

inline constexpr const char *ProjectName = "ESP32 MODULE BASE";
inline constexpr const char *BoardProfileName = "TTGO ESP32 OLED 18650";

struct OledProfile {
    uint8_t sdaPin;
    uint8_t sclPin;
    const u8g2_cb_t *rotation;
};

inline constexpr OledProfile Oled = {
    5,
    4,
    U8G2_R2,
};

} // namespace HardwareProfile
