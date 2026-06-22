#include "MemoryDiagnostics.h"

#include <esp_heap_caps.h>

#include "AppConfig.h"
#include "AppLog.h"

namespace MemoryDiagnostics {

namespace {

constexpr const char *LogTag = "mem";

uint32_t heapFree(uint32_t capabilities)
{
    return static_cast<uint32_t>(heap_caps_get_free_size(capabilities));
}

uint32_t heapMinimumFree(uint32_t capabilities)
{
    return static_cast<uint32_t>(heap_caps_get_minimum_free_size(capabilities));
}

uint32_t heapLargestBlock(uint32_t capabilities)
{
    return static_cast<uint32_t>(heap_caps_get_largest_free_block(capabilities));
}

} // namespace

Snapshot capture()
{
    constexpr uint32_t InternalCaps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    constexpr uint32_t PsramCaps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT;

    Snapshot snapshot = {};
    snapshot.internalFreeBytes = heapFree(InternalCaps);
    snapshot.internalMinimumFreeBytes = heapMinimumFree(InternalCaps);
    snapshot.internalLargestBlockBytes = heapLargestBlock(InternalCaps);
    snapshot.psramTotalBytes = ESP.getPsramSize();
    snapshot.psramFreeBytes = heapFree(PsramCaps);
    snapshot.psramLargestBlockBytes = heapLargestBlock(PsramCaps);
    snapshot.psramAvailable = snapshot.psramTotalBytes > 0 || snapshot.psramFreeBytes > 0;
    return snapshot;
}

void printStartup()
{
    const Snapshot memory = capture();

    APP_LOGI(LogTag, "internal free=%luB min=%luB largest=%luB",
             static_cast<unsigned long>(memory.internalFreeBytes),
             static_cast<unsigned long>(memory.internalMinimumFreeBytes),
             static_cast<unsigned long>(memory.internalLargestBlockBytes));

    if (memory.psramAvailable) {
        APP_LOGI(LogTag, "psram total=%luB free=%luB largest=%luB",
                 static_cast<unsigned long>(memory.psramTotalBytes),
                 static_cast<unsigned long>(memory.psramFreeBytes),
                 static_cast<unsigned long>(memory.psramLargestBlockBytes));
        return;
    }

    APP_LOGI(LogTag, "psram not present or not enabled");
}

void printHeartbeat()
{
    const Snapshot memory = capture();

    APP_LOGI(LogTag, "internal free=%luB largest=%luB",
             static_cast<unsigned long>(memory.internalFreeBytes),
             static_cast<unsigned long>(memory.internalLargestBlockBytes));

    if (memory.internalFreeBytes < AppConfig::LowInternalHeapWarnBytes
        || memory.internalLargestBlockBytes < AppConfig::LowInternalLargestBlockWarnBytes) {
        APP_LOGW(LogTag, "low internal heap headroom");
    }

    if (memory.psramAvailable) {
        APP_LOGI(LogTag, "psram free=%luB largest=%luB",
                 static_cast<unsigned long>(memory.psramFreeBytes),
                 static_cast<unsigned long>(memory.psramLargestBlockBytes));
    }
}

} // namespace MemoryDiagnostics
