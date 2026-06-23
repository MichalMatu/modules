#include "AppTasks.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "DiagnosticsLogger.h"
#include "DisplayRenderer.h"
#include "MqSensors.h"

namespace {

MqSensors mqSensors;
DisplayRenderer displayRenderer(mqSensors);

void mqTask(void *)
{
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        mqSensors.update();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::MqUpdateMs));
    }
}

void displayTask(void *)
{
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        displayRenderer.render();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::ScreenRefreshMs));
    }
}

void diagnosticsTask(void *)
{
    TickType_t lastWake = xTaskGetTickCount();

    for (;;) {
        DiagnosticsLogger::printHeartbeat(mqSensors);
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
    mqSensors.begin();
    displayRenderer.begin();
    DiagnosticsLogger::printStartup();

    const bool mqCreated = createPinnedTask(
        mqTask,
        "mq-sensors",
        AppConfig::MqTaskStack,
        AppConfig::MqTaskPriority,
        AppConfig::MqTaskCore);

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

    if (!mqCreated || !displayCreated || !diagnosticsCreated) {
        Serial.println("[fatal] FreeRTOS task creation failed");
        return false;
    }

    return true;
}
