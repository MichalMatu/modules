#pragma once

#include "GpsSnapshot.h"

namespace DiagnosticsLogger {

void printStartup();
void printSnapshot(const GpsSnapshot &snapshot);

} // namespace DiagnosticsLogger
