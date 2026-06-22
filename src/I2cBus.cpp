#include "I2cBus.h"

#include <Wire.h>

#include "AppConfig.h"

namespace {

SemaphoreHandle_t i2cMutex = nullptr;
bool initialized = false;

} // namespace

namespace I2cBus {

bool begin()
{
    if (i2cMutex == nullptr) {
        i2cMutex = xSemaphoreCreateMutex();
        if (i2cMutex == nullptr) {
            return false;
        }
    }

    if (!initialized) {
        Wire.setPins(AppConfig::OledSdaPin, AppConfig::OledSclPin);
        if (!Wire.begin(AppConfig::OledSdaPin,
                        AppConfig::OledSclPin,
                        AppConfig::I2cClockHz)) {
            return false;
        }
        Wire.setClock(AppConfig::I2cClockHz);
        initialized = true;
    }

    return true;
}

bool lock(TickType_t waitTicks)
{
    return i2cMutex != nullptr && xSemaphoreTake(i2cMutex, waitTicks) == pdTRUE;
}

void unlock()
{
    if (i2cMutex != nullptr) {
        xSemaphoreGive(i2cMutex);
    }
}

} // namespace I2cBus
