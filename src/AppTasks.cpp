#include "AppTasks.h"

#include <Arduino.h>

#include "AppConfig.h"
#include "AppLog.h"
#include "DiagnosticsLogger.h"
#include "DisplayRenderer.h"

namespace {

constexpr const char *LogTag = "tasks";
DisplayRenderer displayRenderer;
TaskHandle_t displayHandle = nullptr;
TaskHandle_t diagnosticsHandle = nullptr;

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
        DiagnosticsLogger::printHeartbeat();
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(AppConfig::DiagnosticLogMs));
    }
}

bool createPinnedTask(TaskFunction_t task,
                      const char *name,
                      uint32_t stackSize,
                      UBaseType_t priority,
                      BaseType_t core,
                      TaskHandle_t *handle)
{
    return xTaskCreatePinnedToCore(
               task,
               name,
               stackSize,
               nullptr,
               priority,
               handle,
               core) == pdPASS;
}

} // namespace

bool startApplicationTasks()
{
    displayRenderer.begin();
    DiagnosticsLogger::printStartup();

    const bool displayCreated = createPinnedTask(
        displayTask,
        "oled-render",
        AppConfig::DisplayTaskStack,
        AppConfig::DisplayTaskPriority,
        AppConfig::DisplayTaskCore,
        &displayHandle);

    const bool diagnosticsCreated = createPinnedTask(
        diagnosticsTask,
        "serial-diag",
        AppConfig::DiagnosticsTaskStack,
        AppConfig::DiagnosticsTaskPriority,
        AppConfig::DiagnosticsTaskCore,
        &diagnosticsHandle);

    if (!displayCreated || !diagnosticsCreated) {
        APP_LOGE(LogTag, "FreeRTOS task creation failed");
        return false;
    }

    APP_LOGI(LogTag, "FreeRTOS tasks started");
    return true;
}

TaskHandle_t displayTaskHandle()
{
    return displayHandle;
}

TaskHandle_t diagnosticsTaskHandle()
{
    return diagnosticsHandle;
}
