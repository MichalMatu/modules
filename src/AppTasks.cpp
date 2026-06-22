#include "AppTasks.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "Cc1101Service.h"
#include "DiagnosticsLogger.h"
#include "DisplayRenderer.h"

namespace {

Cc1101Service cc1101Service;
DisplayRenderer displayRenderer;

void cc1101Task(void *)
{
    for (;;) {
        cc1101Service.poll();
        vTaskDelay(pdMS_TO_TICKS(AppConfig::Cc1101TaskDelayMs));
    }
}

void displayTask(void *)
{
    Cc1101Snapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (cc1101Service.snapshot(snapshot)) {
            displayRenderer.render(snapshot);
        }
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::ScreenRefreshMs));
    }
}

void diagnosticsTask(void *)
{
    Cc1101Snapshot snapshot;
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        if (cc1101Service.snapshot(snapshot)) {
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

    if (!cc1101Service.begin()) {
        Serial.println("[fatal] CC1101 service init failed");
        return false;
    }

    const bool cc1101Created = createPinnedTask(
        cc1101Task,
        "cc1101-rx",
        AppConfig::Cc1101TaskStack,
        AppConfig::Cc1101TaskPriority,
        AppConfig::Cc1101TaskCore);

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

    if (!cc1101Created || !displayCreated || !diagnosticsCreated) {
        Serial.println("[fatal] FreeRTOS task creation failed");
        return false;
    }

    return true;
}
