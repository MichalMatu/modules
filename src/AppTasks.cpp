#include "AppTasks.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "DiagnosticsLogger.h"
#include "DisplayRenderer.h"
#include "Max30100Service.h"

namespace {

Max30100Service max30100Service;
DisplayRenderer displayRenderer;

void max30100Task(void *)
{
    for (;;) {
        max30100Service.poll();
        vTaskDelay(pdMS_TO_TICKS(AppConfig::Max30100TaskDelayMs));
    }
}

void displayTask(void *)
{
    Max30100Snapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (max30100Service.snapshot(snapshot)) {
            displayRenderer.render(snapshot);
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::ScreenRefreshMs));
    }
}

void diagnosticsTask(void *)
{
    Max30100Snapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (max30100Service.snapshot(snapshot)) {
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

    if (!max30100Service.begin()) {
        Serial.println("[fatal] MAX30100 service init failed");
        return false;
    }

    const bool max30100Created = createPinnedTask(
        max30100Task,
        "max30100",
        AppConfig::Max30100TaskStack,
        AppConfig::Max30100TaskPriority,
        AppConfig::Max30100TaskCore);

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

    if (!max30100Created || !displayCreated || !diagnosticsCreated) {
        Serial.println("[fatal] FreeRTOS task creation failed");
        return false;
    }

    return true;
}
