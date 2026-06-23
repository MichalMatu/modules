#include "MqSensors.h"

#include "AppConfig.h"

namespace {

void setupAnalogPin(uint8_t pin)
{
    pinMode(pin, INPUT);
    analogSetPinAttenuation(pin, ADC_11db);
}

} // namespace

void MqSensors::begin()
{
    analogReadResolution(12);

    readings_[Mq2].pin = AppConfig::Mq2AnalogPin;
    readings_[Mq7].pin = AppConfig::Mq7AnalogPin;
    readings_[Mq9].pin = AppConfig::Mq9AnalogPin;

    setupAnalogPin(AppConfig::Mq2AnalogPin);
    setupAnalogPin(AppConfig::Mq7AnalogPin);
    setupAnalogPin(AppConfig::Mq9AnalogPin);

    update();
}

void MqSensors::update()
{
    Reading updated[SensorCount];

    portENTER_CRITICAL(&mux_);
    const bool firstSample = sampleCount_ == 0;

    for (uint8_t i = 0; i < SensorCount; ++i) {
        updated[i] = readings_[i];
    }
    portEXIT_CRITICAL(&mux_);

    for (uint8_t i = 0; i < SensorCount; ++i) {
        const uint16_t raw = analogRead(updated[i].pin);
        updated[i].raw = raw;
        updated[i].percent = rawToPercent(raw);

        if (firstSample) {
            updated[i].minRaw = raw;
            updated[i].maxRaw = raw;
        } else {
            updateStats(updated[i], raw);
        }
    }

    portENTER_CRITICAL(&mux_);
    for (uint8_t i = 0; i < SensorCount; ++i) {
        readings_[i] = updated[i];
    }
    ++sampleCount_;
    lastSampleAtMs_ = millis();
    portEXIT_CRITICAL(&mux_);
}

void MqSensors::resetStats()
{
    portENTER_CRITICAL(&mux_);
    for (uint8_t i = 0; i < SensorCount; ++i) {
        readings_[i].minRaw = readings_[i].raw;
        readings_[i].maxRaw = readings_[i].raw;
    }
    sampleCount_ = 0;
    lastSampleAtMs_ = millis();
    portEXIT_CRITICAL(&mux_);
}

MqSensors::Snapshot MqSensors::snapshot() const
{
    Snapshot data;
    const uint32_t now = millis();

    portENTER_CRITICAL(&mux_);
    for (uint8_t i = 0; i < SensorCount; ++i) {
        data.readings[i] = readings_[i];
    }
    data.sampleCount = sampleCount_;
    data.lastSampleAtMs = lastSampleAtMs_;
    portEXIT_CRITICAL(&mux_);

    if (data.lastSampleAtMs > 0) {
        data.lastSampleAgeMs = now - data.lastSampleAtMs;
    }

    return data;
}

uint8_t MqSensors::rawToPercent(uint16_t raw)
{
    return static_cast<uint8_t>(
        (static_cast<uint32_t>(raw) * 100U) / AppConfig::MqAdcMaxValue);
}

void MqSensors::updateStats(Reading &reading, uint16_t raw)
{
    if (raw < reading.minRaw) {
        reading.minRaw = raw;
    }
    if (raw > reading.maxRaw) {
        reading.maxRaw = raw;
    }
}
