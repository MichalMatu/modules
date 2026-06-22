#include "DiagnosticsLogger.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "AppTasks.h"
#include "AppLog.h"
#include "HardwareProfile.h"
#include "MemoryDiagnostics.h"

namespace {

uint32_t taskStackHighWaterBytes(TaskHandle_t handle)
{
    if (handle == nullptr) {
        return 0;
    }

    return static_cast<uint32_t>(uxTaskGetStackHighWaterMark(handle));
}

void printTaskStack(const char *name, TaskHandle_t handle)
{
    const uint32_t freeBytes = taskStackHighWaterBytes(handle);
    APP_LOGI("stack", "%s free=%luB",
             name,
             static_cast<unsigned long>(freeBytes));

    if (freeBytes > 0 && freeBytes < AppConfig::LowTaskStackWarnBytes) {
        APP_LOGW("stack", "%s stack headroom low", name);
    }
}

} // namespace

namespace DiagnosticsLogger {

void printStartup()
{
    APP_LOGI("startup", "%s", HardwareProfile::ProjectName);
    APP_LOGI("startup", "Board profile: %s", HardwareProfile::BoardProfileName);
    APP_LOGI("startup", "Serial Monitor: %lu",
             static_cast<unsigned long>(AppConfig::SerialBaud));
    APP_LOGI("startup", "OLED I2C SDA=%u SCL=%u",
             HardwareProfile::Oled.sdaPin,
             HardwareProfile::Oled.sclPin);
    APP_LOGI("startup", "Scheduler: FreeRTOS display and diagnostics tasks");
    MemoryDiagnostics::printStartup();
}

void printHeartbeat()
{
    APP_LOGI("diag", "uptime=%lums board=%s oled=sda%u/scl%u",
             static_cast<unsigned long>(millis()),
             HardwareProfile::BoardProfileName,
             HardwareProfile::Oled.sdaPin,
             HardwareProfile::Oled.sclPin);
    MemoryDiagnostics::printHeartbeat();
    printTaskStack("oled-render", displayTaskHandle());
    printTaskStack("serial-diag", diagnosticsTaskHandle());
}

} // namespace DiagnosticsLogger
