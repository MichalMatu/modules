#include <Arduino.h>

#include "AppConfig.h"
#include "AppTasks.h"

void setup()
{
    Serial.begin(AppConfig::SerialBaud);
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!startApplicationTasks()) {
        for (;;) {
            Serial.println("[fatal] Application init failed");
            vTaskDelay(pdMS_TO_TICKS(AppConfig::DiagnosticLogMs));
        }
    }
}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(AppConfig::IdleTaskDelayMs));
}
