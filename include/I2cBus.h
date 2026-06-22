#pragma once

#include <Arduino.h>

namespace I2cBus {

bool begin();
bool lock(TickType_t waitTicks = portMAX_DELAY);
void unlock();

} // namespace I2cBus
