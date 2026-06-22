#include "GpsService.h"

#include <ctype.h>
#include <string.h>

#include "AppConfig.h"

GpsService::GpsService()
    : serial_(2)
{
}

bool GpsService::begin()
{
    snapshotMutex_ = xSemaphoreCreateMutex();
    if (snapshotMutex_ == nullptr) {
        return false;
    }

    if (!autoDetectBaud()) {
        beginSerial(AppConfig::GpsBaudRates[0]);
        Serial.println("[gps] no NMEA detected during baud scan");
        Serial.printf("[gps] listening at %lu and will rescan every %lums\n",
                      activeBaud_,
                      AppConfig::GpsBaudRescanMs);
    }

    publishSnapshot();
    return true;
}

void GpsService::poll()
{
    if (!baudDetected_ &&
        millis() - lastBaudScanAtMs_ >= AppConfig::GpsBaudRescanMs) {
        autoDetectBaud();
    }

    while (serial_.available() > 0) {
        const char c = static_cast<char>(serial_.read());
        parser_.encode(c);

        if (parser_.passedChecksum() > 0) {
            baudDetected_ = true;
        }

        if (AppConfig::SerialRawNmea && baudDetected_) {
            Serial.write(c);
        }
    }

    publishSnapshot();
}

bool GpsService::snapshot(GpsSnapshot &out, TickType_t waitTicks)
{
    if (snapshotMutex_ == nullptr) {
        return false;
    }

    if (xSemaphoreTake(snapshotMutex_, waitTicks) != pdTRUE) {
        return false;
    }

    out = snapshot_;
    xSemaphoreGive(snapshotMutex_);
    return true;
}

bool GpsService::autoDetectBaud()
{
    lastBaudScanAtMs_ = millis();

    for (size_t i = 0; i < AppConfig::GpsBaudRateCount; ++i) {
        const uint32_t baud = AppConfig::GpsBaudRates[i];

        if (probeBaud(baud)) {
            baudDetected_ = true;
            Serial.printf("[gps] NMEA detected at %lu baud\n", baud);
            publishSnapshot();
            return true;
        }
    }

    baudDetected_ = false;
    publishSnapshot();
    return false;
}

bool GpsService::probeBaud(uint32_t baud)
{
    beginSerial(baud);

    while (serial_.available() > 0) {
        serial_.read();
    }

    char sentence[96] = {};
    size_t length = 0;
    bool collecting = false;
    const uint32_t startedAt = millis();

    while (millis() - startedAt < AppConfig::GpsBaudProbeMs) {
        while (serial_.available() > 0) {
            const char c = static_cast<char>(serial_.read());

            if (c == '$') {
                collecting = true;
                length = 0;
                sentence[length++] = c;
                continue;
            }

            if (!collecting) {
                continue;
            }

            if (c == '\n' || c == '\r') {
                sentence[length] = '\0';
                if (isNmeaSentence(sentence)) {
                    return true;
                }
                collecting = false;
                length = 0;
                continue;
            }

            if (!isprint(static_cast<unsigned char>(c)) || length >= sizeof(sentence) - 1) {
                collecting = false;
                length = 0;
                continue;
            }

            sentence[length++] = c;
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return false;
}

void GpsService::beginSerial(uint32_t baud)
{
    if (activeBaud_ != 0) {
        serial_.end();
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    activeBaud_ = baud;
    serial_.setRxBufferSize(AppConfig::GpsRxBufferSize);
    serial_.begin(activeBaud_,
                  SERIAL_8N1,
                  AppConfig::GpsRxPin,
                  AppConfig::GpsTxPin);
}

bool GpsService::isNmeaSentence(const char *sentence) const
{
    const size_t length = strlen(sentence);
    if (length < 12 || length > 90) {
        return false;
    }

    if (sentence[0] != '$' || sentence[1] != 'G') {
        return false;
    }

    const char talker = sentence[2];
    if (talker != 'P' && talker != 'N' && talker != 'L' &&
        talker != 'A' && talker != 'B' && talker != 'Q') {
        return false;
    }

    return strchr(sentence, ',') != nullptr;
}

void GpsService::publishSnapshot()
{
    if (snapshotMutex_ == nullptr) {
        return;
    }

    GpsSnapshot next;
    next.uptimeMs = millis();
    next.activeBaud = activeBaud_;
    next.baudDetected = baudDetected_;
    next.charsProcessed = parser_.charsProcessed();
    next.passedChecksum = parser_.passedChecksum();
    next.failedChecksum = parser_.failedChecksum();
    next.dataReceived = baudDetected_ || next.passedChecksum > 0;

    next.satellites = parser_.satellites.isValid() ? parser_.satellites.value() : 0;
    next.hdop = parser_.hdop.isValid() ? parser_.hdop.hdop() : 0.0;

    next.locationValid = parser_.location.isValid();
    next.locationAgeMs = next.locationValid ? parser_.location.age() : UINT32_MAX;
    next.freshFix = next.locationValid &&
                    next.locationAgeMs <= AppConfig::FreshFixMaxAgeMs;

    if (next.locationValid) {
        next.latitude = parser_.location.lat();
        next.longitude = parser_.location.lng();
    }

    if (parser_.speed.isValid()) {
        next.speedKmph = parser_.speed.kmph();
    }

    if (parser_.altitude.isValid()) {
        next.altitudeMeters = parser_.altitude.meters();
    }

    next.timeValid = parser_.time.isValid();
    if (next.timeValid) {
        next.hour = parser_.time.hour();
        next.minute = parser_.time.minute();
        next.second = parser_.time.second();
    }

    next.dateValid = parser_.date.isValid();
    if (next.dateValid) {
        next.day = parser_.date.day();
        next.month = parser_.date.month();
        next.year = parser_.date.year();
    }

    if (xSemaphoreTake(snapshotMutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot_ = next;
        xSemaphoreGive(snapshotMutex_);
    }
}
