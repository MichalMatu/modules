#pragma once

#include <Arduino.h>

namespace MemoryDiagnostics {

struct Snapshot {
    uint32_t internalFreeBytes;
    uint32_t internalMinimumFreeBytes;
    uint32_t internalLargestBlockBytes;
    uint32_t psramTotalBytes;
    uint32_t psramFreeBytes;
    uint32_t psramLargestBlockBytes;
    bool psramAvailable;
};

Snapshot capture();
void printStartup();
void printHeartbeat();

} // namespace MemoryDiagnostics
