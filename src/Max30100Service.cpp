#include "Max30100Service.h"

#include "AppConfig.h"
#include "I2cBus.h"

Max30100Service *Max30100Service::activeService_ = nullptr;

Max30100Service::Max30100Service() = default;

bool Max30100Service::begin()
{
    snapshotMutex_ = xSemaphoreCreateMutex();
    if (snapshotMutex_ == nullptr) {
        return false;
    }

    if (!I2cBus::begin()) {
        return false;
    }

    activeService_ = this;
    initializeSensor();
    publishSnapshot();
    return true;
}

void Max30100Service::poll()
{
    const uint32_t now = millis();

    if (!sensorFound_) {
        if (now - lastInitAttemptAtMs_ >= AppConfig::Max30100RetryMs) {
            initializeSensor();
            publishSnapshot();
        }
        return;
    }

    if (I2cBus::lock(pdMS_TO_TICKS(50))) {
        pulseOximeter_.update();
        I2cBus::unlock();
    }

    if (now - lastPublishAtMs_ >= AppConfig::Max30100PublishMs) {
        publishSnapshot();
    }
}

bool Max30100Service::snapshot(Max30100Snapshot &out, TickType_t waitTicks)
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

bool Max30100Service::initializeSensor()
{
    lastInitAttemptAtMs_ = millis();

    if (!I2cBus::lock(pdMS_TO_TICKS(250))) {
        sensorFound_ = false;
        Serial.println("[max30100] I2C bus busy during init");
        return false;
    }

    const bool ready = pulseOximeter_.begin();
    if (ready) {
        pulseOximeter_.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
        pulseOximeter_.setOnBeatDetectedCallback(handleBeatDetected);
        Serial.printf("[max30100] sensor detected at 0x%02X\n",
                      AppConfig::Max30100Address);
    } else {
        Serial.printf("[max30100] sensor not found at 0x%02X\n",
                      AppConfig::Max30100Address);
    }

    I2cBus::unlock();
    sensorFound_ = ready;
    return ready;
}

void Max30100Service::publishSnapshot()
{
    lastPublishAtMs_ = millis();

    Max30100Snapshot next;
    next.uptimeMs = lastPublishAtMs_;
    next.sensorFound = sensorFound_;
    next.lastInitAttemptAtMs = lastInitAttemptAtMs_;
    next.lastBeatAtMs = lastBeatAtMs_;
    next.beatCount = beatCount_;
    next.freshBeat = sensorFound_ &&
                     lastBeatAtMs_ != 0 &&
                     lastPublishAtMs_ - lastBeatAtMs_ <= AppConfig::BeatIndicatorMs;

    if (sensorFound_) {
        next.heartRateBpm = pulseOximeter_.getHeartRate();
        next.spo2 = pulseOximeter_.getSpO2();
        next.redLedCurrentBias = pulseOximeter_.getRedLedCurrentBias();
        next.heartRateValid = next.heartRateBpm > 0.0f && next.heartRateBpm < 250.0f;
        next.spo2Valid = next.spo2 > 0 && next.spo2 <= 100;
    }

    if (xSemaphoreTake(snapshotMutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot_ = next;
        xSemaphoreGive(snapshotMutex_);
    }
}

void Max30100Service::handleBeatDetected()
{
    if (activeService_ == nullptr) {
        return;
    }

    ++activeService_->beatCount_;
    activeService_->lastBeatAtMs_ = millis();
}
