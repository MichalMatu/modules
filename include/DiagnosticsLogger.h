#pragma once

#include "Max30100Snapshot.h"

namespace DiagnosticsLogger {

void printStartup();
void printSnapshot(const Max30100Snapshot &snapshot);

} // namespace DiagnosticsLogger
