#include <Arduino.h>

#include "AppConfig.h"
#include "AppLog.h"
#include "AppTasks.h"

void setup()
{
    Serial.begin(AppConfig::SerialBaud);
    AppLog::begin(Serial);
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!startApplicationTasks()) {
        for (;;) {
            APP_LOGE("main", "Application init failed");
            vTaskDelay(pdMS_TO_TICKS(AppConfig::DiagnosticLogMs));
        }
    }
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(AppConfig::IdleTaskDelayMs));
}
