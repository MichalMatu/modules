#include "AppTasks.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "DiagnosticsLogger.h"
#include "DisplayRenderer.h"
#include "GpsService.h"

namespace {

GpsService gpsService;
DisplayRenderer displayRenderer;

void gpsTask(void *)
{
    for (;;) {
        gpsService.poll();
        vTaskDelay(pdMS_TO_TICKS(AppConfig::GpsTaskDelayMs));
    }
}

void displayTask(void *)
{
    GpsSnapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (gpsService.snapshot(snapshot)) {
            displayRenderer.render(snapshot);
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::ScreenRefreshMs));
    }
}

void diagnosticsTask(void *)
{
    GpsSnapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (gpsService.snapshot(snapshot)) {
            DiagnosticsLogger::printSnapshot(snapshot);
        }

        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::DiagnosticLogMs));
    }
}

bool createPinnedTask(TaskFunction_t task,
                      const char *name,
                      uint32_t stackSize,
                      UBaseType_t priority,
                      BaseType_t core)
{
    return xTaskCreatePinnedToCore(
               task,
               name,
               stackSize,
               nullptr,
               priority,
               nullptr,
               core) == pdPASS;
}

} // namespace

bool startApplicationTasks()
{
    displayRenderer.begin();
    DiagnosticsLogger::printStartup();

    if (!gpsService.begin()) {
        Serial.println("[fatal] GPS service init failed");
        return false;
    }

    const bool gpsCreated = createPinnedTask(
        gpsTask,
        "gps-uart",
        AppConfig::GpsTaskStack,
        AppConfig::GpsTaskPriority,
        AppConfig::GpsTaskCore);

    const bool displayCreated = createPinnedTask(
        displayTask,
        "oled-render",
        AppConfig::DisplayTaskStack,
        AppConfig::DisplayTaskPriority,
        AppConfig::DisplayTaskCore);

    const bool diagnosticsCreated = createPinnedTask(
        diagnosticsTask,
        "serial-diag",
        AppConfig::DiagnosticsTaskStack,
        AppConfig::DiagnosticsTaskPriority,
        AppConfig::DiagnosticsTaskCore);

    if (!gpsCreated || !displayCreated || !diagnosticsCreated) {
        Serial.println("[fatal] FreeRTOS task creation failed");
        return false;
    }

    return true;
}
