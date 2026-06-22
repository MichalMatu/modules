#include "Cc1101Service.h"

#include <SPI.h>
#include <string.h>

#include "AppConfig.h"

namespace {

constexpr uint32_t RecentCodeMs = 3000;
constexpr unsigned int MaxRawTimingCount = 24;

void valueToBinary(unsigned long value, unsigned int bitLength, char *out, size_t outSize)
{
    if (outSize == 0) {
        return;
    }

    out[0] = '\0';
    if (bitLength == 0) {
        return;
    }

    const unsigned int printableBits = bitLength > 32 ? 32 : bitLength;
    const size_t charsToWrite = printableBits < outSize - 1 ? printableBits : outSize - 1;
    for (size_t i = 0; i < charsToWrite; ++i) {
        const unsigned int shift = printableBits - 1 - i;
        out[i] = (value & (1UL << shift)) ? '1' : '0';
    }
    out[charsToWrite] = '\0';
}

} // namespace

Cc1101Service::Cc1101Service()
    : module_(AppConfig::Cc1101CsnPin,
              AppConfig::Cc1101Gdo0Pin,
              RADIOLIB_NC,
              AppConfig::Cc1101Gdo2Pin),
      radio_(&module_)
{
}

bool Cc1101Service::begin()
{
    snapshotMutex_ = xSemaphoreCreateMutex();
    if (snapshotMutex_ == nullptr) {
        return false;
    }

    SPI.begin(AppConfig::Cc1101SckPin,
              AppConfig::Cc1101MisoPin,
              AppConfig::Cc1101MosiPin,
              AppConfig::Cc1101CsnPin);

    initializeRadio();
    publishSnapshot();
    return true;
}

void Cc1101Service::poll()
{
    const uint32_t now = millis();

    if (!radioReady_) {
        if (now - lastInitAttemptAtMs_ >= AppConfig::Cc1101RetryMs) {
            initializeRadio();
            publishSnapshot();
        }
        return;
    }

    refreshRssi();

    if (rcSwitch_.available()) {
        handleReceivedCode();
    }

    if (!listening_ && now - lastReceiveAttemptAtMs_ >= AppConfig::Cc1101RetryMs) {
        restartReceive();
    }

    publishSnapshot();
}

bool Cc1101Service::snapshot(Cc1101Snapshot &out, TickType_t waitTicks)
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

bool Cc1101Service::initializeRadio()
{
    if (rcSwitchEnabled_) {
        rcSwitch_.disableReceive();
        rcSwitchEnabled_ = false;
    }

    lastInitAttemptAtMs_ = millis();
    listening_ = false;

    ConfigFSK_t config;
    config.frequency = AppConfig::Cc1101FrequencyMhz;
    config.bitRate = AppConfig::Cc1101OokBitRateKbps;
    config.frequencyDeviation = 5.0f;
    config.receiverBandwidth = AppConfig::Cc1101OokRxBandwidthKhz;
    config.power = AppConfig::Cc1101OutputPowerDbm;
    config.preambleLength = 16;

    lastState_ = radio_.begin(config);
    radioReady_ = lastState_ == RADIOLIB_ERR_NONE;

    if (!radioReady_) {
        Serial.printf("[cc1101] init failed code=%d\n", lastState_);
        return false;
    }

    lastState_ = radio_.setOOK(true);
    if (lastState_ != RADIOLIB_ERR_NONE) {
        radioReady_ = false;
        Serial.printf("[cc1101] OOK setup failed code=%d\n", lastState_);
        return false;
    }

    lastState_ = radio_.setRxBandwidth(AppConfig::Cc1101OokRxBandwidthKhz);
    if (lastState_ != RADIOLIB_ERR_NONE) {
        radioReady_ = false;
        Serial.printf("[cc1101] RX bandwidth setup failed code=%d\n", lastState_);
        return false;
    }

    restartReceive();
    Serial.printf("[cc1101] OOK/ASK rc-switch %.2f MHz, br=%.1f kbps, rx_bw=%.1f kHz\n",
                  AppConfig::Cc1101FrequencyMhz,
                  AppConfig::Cc1101OokBitRateKbps,
                  AppConfig::Cc1101OokRxBandwidthKhz);
    return true;
}

void Cc1101Service::handleReceivedCode()
{
    const uint32_t now = millis();
    const unsigned long value = rcSwitch_.getReceivedValue();
    const unsigned int bitLength = rcSwitch_.getReceivedBitlength();
    const unsigned int delayUs = rcSwitch_.getReceivedDelay();
    const unsigned int protocol = rcSwitch_.getReceivedProtocol();
    const unsigned int *rawTimings = rcSwitch_.getReceivedRawdata();

    const bool repeated = lastCodeAtMs_ != 0 &&
                          value == lastValue_ &&
                          bitLength == lastBitLength_ &&
                          protocol == lastProtocol_ &&
                          now - lastCodeAtMs_ <= AppConfig::RcSwitchRepeatWindowMs;

    ++receivedCodeCount_;
    repeatCount_ = repeated ? repeatCount_ + 1 : 1;
    lastCodeAtMs_ = now;
    lastCodeRssiDbm_ = lastRssiDbm_;
    lastValue_ = value;
    lastBitLength_ = static_cast<uint8_t>(bitLength > 255 ? 255 : bitLength);
    lastDelayUs_ = static_cast<uint16_t>(delayUs > 65535 ? 65535 : delayUs);
    lastProtocol_ = static_cast<uint8_t>(protocol > 255 ? 255 : protocol);

    valueToBinary(lastValue_, bitLength, lastBinary_, sizeof(lastBinary_));
    buildRawPreview(rawTimings, bitLength);

    Serial.printf("[rcswitch] value=%lu bits=%u protocol=%u delay=%uus repeat=%u rssi=%.1fdBm binary=%s raw=%s\n",
                  lastValue_,
                  bitLength,
                  protocol,
                  delayUs,
                  repeatCount_,
                  lastCodeRssiDbm_,
                  lastBinary_,
                  rawPreview_);

    rcSwitch_.resetAvailable();
}

void Cc1101Service::buildRawPreview(const unsigned int *rawTimings, unsigned int bitLength)
{
    rawPreview_[0] = '\0';

    if (rawTimings == nullptr) {
        return;
    }

    const unsigned int wantedCount = bitLength * 2 + 1;
    const unsigned int timingCount = wantedCount < MaxRawTimingCount ? wantedCount : MaxRawTimingCount;
    size_t offset = 0;

    for (unsigned int i = 0; i < timingCount && offset + 1 < sizeof(rawPreview_); ++i) {
        const int written = snprintf(rawPreview_ + offset,
                                     sizeof(rawPreview_) - offset,
                                     "%u%s",
                                     rawTimings[i],
                                     i + 1 < timingCount ? "," : "");
        if (written <= 0) {
            break;
        }

        if (static_cast<size_t>(written) >= sizeof(rawPreview_) - offset) {
            rawPreview_[sizeof(rawPreview_) - 1] = '\0';
            break;
        }
        offset += static_cast<size_t>(written);
    }
}

void Cc1101Service::publishSnapshot()
{
    Cc1101Snapshot next;
    const uint32_t now = millis();

    next.uptimeMs = now;
    next.radioReady = radioReady_;
    next.listening = listening_;
    next.recentCode = lastCodeAtMs_ != 0 && now - lastCodeAtMs_ <= RecentCodeMs;
    next.lastInitAttemptAtMs = lastInitAttemptAtMs_;
    next.lastCodeAtMs = lastCodeAtMs_;
    next.receivedCodeCount = receivedCodeCount_;
    next.lastState = lastState_;
    next.frequencyMhz = AppConfig::Cc1101FrequencyMhz;
    next.rssiDbm = lastRssiDbm_;
    next.noiseFloorDbm = noiseFloorDbm_;
    next.lastCodeRssiDbm = lastCodeRssiDbm_;
    next.lastValue = static_cast<uint32_t>(lastValue_);
    next.repeatCount = repeatCount_;
    next.lastDelayUs = lastDelayUs_;
    next.lastBitLength = lastBitLength_;
    next.lastProtocol = lastProtocol_;
    strlcpy(next.lastBinary, lastBinary_, sizeof(next.lastBinary));

    if (xSemaphoreTake(snapshotMutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot_ = next;
        xSemaphoreGive(snapshotMutex_);
    }
}

void Cc1101Service::refreshRssi()
{
    if (!radioReady_) {
        return;
    }

    lastRssiDbm_ = radio_.getRSSI();
    if (lastCodeAtMs_ == 0 || millis() - lastCodeAtMs_ > RecentCodeMs) {
        noiseFloorDbm_ = noiseFloorDbm_ * 0.95f + lastRssiDbm_ * 0.05f;
    }
}

void Cc1101Service::restartReceive()
{
    if (rcSwitchEnabled_) {
        rcSwitch_.disableReceive();
        rcSwitchEnabled_ = false;
    }

    lastReceiveAttemptAtMs_ = millis();
    pinMode(AppConfig::Cc1101Gdo0Pin, INPUT);

    lastState_ = radio_.receiveDirectAsync();
    listening_ = lastState_ == RADIOLIB_ERR_NONE;
    if (!listening_) {
        Serial.printf("[cc1101] direct receive failed code=%d\n", lastState_);
        return;
    }

    rcSwitch_.resetAvailable();
    rcSwitch_.enableReceive(AppConfig::Cc1101Gdo0Pin);
    rcSwitchEnabled_ = true;
    Serial.printf("[cc1101] rc-switch receive on GDO0 GPIO%u\n", AppConfig::Cc1101Gdo0Pin);
}
