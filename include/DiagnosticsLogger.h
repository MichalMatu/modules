#pragma once

#include "Cc1101Snapshot.h"

namespace DiagnosticsLogger {

void printStartup();
void printSnapshot(const Cc1101Snapshot &snapshot);

} // namespace DiagnosticsLogger
