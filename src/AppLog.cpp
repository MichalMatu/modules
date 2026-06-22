#include "AppLog.h"

#include <cstdarg>
#include <cstdio>

namespace AppLog {

namespace {

HardwareSerial *logSerial = nullptr;
Level currentLevel = CompileTimeLevel;

const char *levelName(Level level)
{
    switch (level) {
    case Level::Error:
        return "E";
    case Level::Warn:
        return "W";
    case Level::Info:
        return "I";
    case Level::Debug:
        return "D";
    case Level::Verbose:
        return "V";
    }

    return "?";
}

} // namespace

void begin(HardwareSerial &serial)
{
    logSerial = &serial;
}

void setRuntimeLevel(Level level)
{
    currentLevel = level;
}

Level runtimeLevel()
{
    return currentLevel;
}

bool enabled(Level level)
{
    return static_cast<uint8_t>(level) <= static_cast<uint8_t>(CompileTimeLevel)
        && static_cast<uint8_t>(level) <= static_cast<uint8_t>(currentLevel);
}

void write(Level level, const char *tag, const char *format, ...)
{
    if (logSerial == nullptr || !enabled(level)) {
        return;
    }

    char message[160];
    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    logSerial->print('[');
    logSerial->print(static_cast<unsigned long>(millis()));
    logSerial->print("] ");
    logSerial->print(levelName(level));
    logSerial->print('/');
    logSerial->print(tag);
    logSerial->print(": ");
    logSerial->println(message);
}

} // namespace AppLog
