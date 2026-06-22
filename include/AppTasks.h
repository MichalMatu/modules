#pragma once

#include <Arduino.h>

bool startApplicationTasks();
TaskHandle_t displayTaskHandle();
TaskHandle_t diagnosticsTaskHandle();
