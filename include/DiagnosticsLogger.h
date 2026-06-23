#pragma once

#include "MqSensors.h"

namespace DiagnosticsLogger {

void printStartup();
void printHeartbeat(const MqSensors &sensors);

} // namespace DiagnosticsLogger
