#pragma once

#include <Arduino.h>

namespace AppLog {

enum class Level : uint8_t {
    Error = 0,
    Warn = 1,
    Info = 2,
    Debug = 3,
    Verbose = 4,
};

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL 2
#endif

inline constexpr Level CompileTimeLevel = static_cast<Level>(APP_LOG_LEVEL);

void begin(HardwareSerial &serial);
void setRuntimeLevel(Level level);
Level runtimeLevel();
bool enabled(Level level);
void write(Level level, const char *tag, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

} // namespace AppLog

#if APP_LOG_LEVEL >= 0
#define APP_LOGE(tag, format, ...) \
    do { \
        if (AppLog::enabled(AppLog::Level::Error)) { \
            AppLog::write(AppLog::Level::Error, tag, format, ##__VA_ARGS__); \
        } \
    } while (false)
#else
#define APP_LOGE(tag, format, ...) do {} while (false)
#endif

#if APP_LOG_LEVEL >= 1
#define APP_LOGW(tag, format, ...) \
    do { \
        if (AppLog::enabled(AppLog::Level::Warn)) { \
            AppLog::write(AppLog::Level::Warn, tag, format, ##__VA_ARGS__); \
        } \
    } while (false)
#else
#define APP_LOGW(tag, format, ...) do {} while (false)
#endif

#if APP_LOG_LEVEL >= 2
#define APP_LOGI(tag, format, ...) \
    do { \
        if (AppLog::enabled(AppLog::Level::Info)) { \
            AppLog::write(AppLog::Level::Info, tag, format, ##__VA_ARGS__); \
        } \
    } while (false)
#else
#define APP_LOGI(tag, format, ...) do {} while (false)
#endif

#if APP_LOG_LEVEL >= 3
#define APP_LOGD(tag, format, ...) \
    do { \
        if (AppLog::enabled(AppLog::Level::Debug)) { \
            AppLog::write(AppLog::Level::Debug, tag, format, ##__VA_ARGS__); \
        } \
    } while (false)
#else
#define APP_LOGD(tag, format, ...) do {} while (false)
#endif

#if APP_LOG_LEVEL >= 4
#define APP_LOGV(tag, format, ...) \
    do { \
        if (AppLog::enabled(AppLog::Level::Verbose)) { \
            AppLog::write(AppLog::Level::Verbose, tag, format, ##__VA_ARGS__); \
        } \
    } while (false)
#else
#define APP_LOGV(tag, format, ...) do {} while (false)
#endif
