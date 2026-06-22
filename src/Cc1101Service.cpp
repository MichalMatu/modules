#include "Cc1101Service.h"

#include <SPI.h>
#include <driver/gpio.h>
#include <string.h>

#include "AppConfig.h"

namespace {

constexpr size_t CapturedPulseBufferSize = 512;
constexpr uint32_t RecentBurstMs = 2500;
constexpr uint16_t MaxStoredPulseUs = 65535;

volatile uint16_t pulseDurations_[CapturedPulseBufferSize] = {};
volatile uint8_t pulseLevels_[CapturedPulseBufferSize] = {};
volatile uint16_t pulseHead_ = 0;
volatile uint16_t pulseTail_ = 0;
volatile uint32_t pulseOverflowCount_ = 0;
volatile uint32_t lastEdgeAtUs_ = 0;
volatile uint8_t lastGdo0Level_ = 0;
portMUX_TYPE pulseMux_ = portMUX_INITIALIZER_UNLOCKED;

bool isNear(uint16_t value, uint16_t target, uint16_t tolerance)
{
    const uint16_t low = target > tolerance ? target - tolerance : 0;
    const uint16_t high = target + tolerance;
    return value >= low && value <= high;
}

void sortDurations(uint16_t *values, size_t count)
{
    for (size_t i = 1; i < count; ++i) {
        const uint16_t value = values[i];
        size_t j = i;
        while (j > 0 && values[j - 1] > value) {
            values[j] = values[j - 1];
            --j;
        }
        values[j] = value;
    }
}

uint8_t classifyUnits(uint16_t durationUs, uint16_t unitUs)
{
    if (unitUs == 0) {
        return 0;
    }

    for (uint8_t units = 1; units <= 4; ++units) {
        const uint16_t target = unitUs * units;
        const uint16_t tolerance = units >= 3 ? unitUs : unitUs / 2;
        if (isNear(durationUs, target, tolerance)) {
            return units;
        }
    }

    return 0;
}

char classifyPair(uint16_t activeUs, uint16_t idleUs, uint16_t unitUs)
{
    const uint8_t activeUnits = classifyUnits(activeUs, unitUs);
    const uint8_t idleUnits = classifyUnits(idleUs, unitUs);

    if (activeUnits == 1 && idleUnits >= 2) {
        return '0';
    }
    if (activeUnits >= 2 && idleUnits == 1) {
        return '1';
    }
    if (activeUnits == 1 && idleUnits == 1) {
        return 'F';
    }
    return '?';
}

void bitsToHex(const char *bits, uint8_t bitCount, char *out, size_t outSize)
{
    if (outSize == 0) {
        return;
    }

    uint64_t value = 0;
    for (uint8_t i = 0; i < bitCount; ++i) {
        if (bits[i] != '0' && bits[i] != '1') {
            out[0] = '\0';
            return;
        }
        value = (value << 1) | (bits[i] == '1' ? 1ULL : 0ULL);
    }

    const uint8_t hexDigits = (bitCount + 3) / 4;
    snprintf(out, outSize, "0x%0*llX", hexDigits, static_cast<unsigned long long>(value));
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
    processCapturedPulses();

    if (burstPulseCount_ > 0 &&
        now - lastBurstPulseAtMs_ >= AppConfig::OokStaleBurstMs) {
        finalizeBurst();
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
    if (captureAttached_) {
        detachInterrupt(digitalPinToInterrupt(AppConfig::Cc1101Gdo0Pin));
        captureAttached_ = false;
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
    Serial.printf("[cc1101] OOK/ASK sniff %.2f MHz, br=%.1f kbps, rx_bw=%.1f kHz\n",
                  AppConfig::Cc1101FrequencyMhz,
                  AppConfig::Cc1101OokBitRateKbps,
                  AppConfig::Cc1101OokRxBandwidthKhz);
    return true;
}

void Cc1101Service::processCapturedPulses()
{
    portENTER_CRITICAL(&pulseMux_);
    edgeOverflowCount_ = pulseOverflowCount_;
    portEXIT_CRITICAL(&pulseMux_);

    uint8_t level = 0;
    uint16_t durationUs = 0;
    while (popCapturedPulse(level, durationUs)) {
        handlePulse(level, durationUs);
    }
}

bool Cc1101Service::popCapturedPulse(uint8_t &level, uint16_t &durationUs)
{
    bool hasPulse = false;

    portENTER_CRITICAL(&pulseMux_);
    if (pulseTail_ != pulseHead_) {
        durationUs = pulseDurations_[pulseTail_];
        level = pulseLevels_[pulseTail_];
        pulseTail_ = (pulseTail_ + 1) % CapturedPulseBufferSize;
        hasPulse = true;
    }
    portEXIT_CRITICAL(&pulseMux_);

    return hasPulse;
}

void Cc1101Service::handlePulse(uint8_t level, uint16_t durationUs)
{
    if (durationUs < AppConfig::OokMinPulseUs) {
        return;
    }

    if (durationUs >= AppConfig::OokBurstGapUs) {
        finalizeBurst();
        return;
    }

    if (durationUs > AppConfig::OokMaxPulseUs) {
        return;
    }

    if (burstPulseCount_ == 0) {
        burstPeakRssiDbm_ = lastRssiDbm_;
        burstOverflowed_ = false;
    } else if (lastRssiDbm_ > burstPeakRssiDbm_) {
        burstPeakRssiDbm_ = lastRssiDbm_;
    }

    if (burstPulseCount_ < MaxBurstPulses) {
        burst_[burstPulseCount_].level = level;
        burst_[burstPulseCount_].durationUs = durationUs;
        ++burstPulseCount_;
    } else {
        burstOverflowed_ = true;
    }

    lastBurstPulseAtMs_ = millis();
}

void Cc1101Service::finalizeBurst()
{
    if (burstPulseCount_ == 0) {
        return;
    }

    uint32_t totalUs = 0;
    for (size_t i = 0; i < burstPulseCount_; ++i) {
        totalUs += burst_[i].durationUs;
    }

    const bool enoughPulses = burstPulseCount_ >= AppConfig::OokMinBurstPulses;
    const bool enoughSignal = burstPeakRssiDbm_ >= AppConfig::OokMinBurstRssiDbm;

    if (!enoughPulses || !enoughSignal || burstOverflowed_) {
        ++rejectedBurstCount_;
        burstPulseCount_ = 0;
        burstPeakRssiDbm_ = -140.0f;
        burstOverflowed_ = false;
        return;
    }

    DecodeResult decoded;
    const bool hasDecode = decodeBurst(decoded);
    if (decoded.unitUs < AppConfig::OokMinUnitUs || decoded.unitUs > AppConfig::OokMaxUnitUs) {
        ++rejectedBurstCount_;
        burstPulseCount_ = 0;
        burstPeakRssiDbm_ = -140.0f;
        burstOverflowed_ = false;
        return;
    }

    const uint32_t now = millis();
    const bool repeated = hasDecode &&
                          strcmp(decoded.bits, lastBits_) == 0 &&
                          now - lastBurstAtMs_ <= AppConfig::OokRepeatWindowMs;

    ++burstCount_;
    if (hasDecode) {
        ++decodedBurstCount_;
    }

    repeatCount_ = repeated ? repeatCount_ + 1 : 1;
    lastBurstAtMs_ = now;
    lastBurstRssiDbm_ = burstPeakRssiDbm_;
    lastPulseCount_ = static_cast<uint16_t>(burstPulseCount_);
    lastBurstDurationMs_ = static_cast<uint16_t>(totalUs / 1000);
    lastDecoded_ = hasDecode;
    lastInverted_ = decoded.inverted;
    lastBitCount_ = decoded.bitCount;
    lastUnitUs_ = decoded.unitUs;
    buildRawPreview(rawPreview_, sizeof(rawPreview_));

    if (hasDecode) {
        strlcpy(lastBits_, decoded.bits, sizeof(lastBits_));
        strlcpy(lastHex_, decoded.hex, sizeof(lastHex_));
        Serial.printf("[ook] code bits=%u unit=%uus pulses=%u rssi=%.1fdBm repeat=%u inv=%s hex=%s bits=%s raw=%s\n",
                      lastBitCount_,
                      lastUnitUs_,
                      lastPulseCount_,
                      lastBurstRssiDbm_,
                      repeatCount_,
                      lastInverted_ ? "yes" : "no",
                      lastHex_[0] != '\0' ? lastHex_ : "-",
                      lastBits_,
                      rawPreview_);
    } else {
        lastBits_[0] = '\0';
        lastHex_[0] = '\0';
        Serial.printf("[ook] burst pulses=%u unit=%uus rssi=%.1fdBm raw=%s\n",
                      lastPulseCount_,
                      lastUnitUs_,
                      lastBurstRssiDbm_,
                      rawPreview_);
    }

    burstPulseCount_ = 0;
    burstPeakRssiDbm_ = -140.0f;
    burstOverflowed_ = false;
}

bool Cc1101Service::decodeBurst(DecodeResult &result) const
{
    uint16_t durations[MaxBurstPulses];
    size_t durationCount = 0;
    for (size_t i = 0; i < burstPulseCount_ && durationCount < MaxBurstPulses; ++i) {
        const uint16_t duration = burst_[i].durationUs;
        if (duration >= AppConfig::OokMinPulseUs && duration <= AppConfig::OokMaxPulseUs) {
            durations[durationCount++] = duration;
        }
    }

    if (durationCount < AppConfig::OokMinBurstPulses) {
        return false;
    }

    sortDurations(durations, durationCount);

    const uint16_t shortest = durations[durationCount / 10];
    uint32_t unitSum = 0;
    size_t unitCount = 0;
    const uint16_t sameClusterLimit = shortest + shortest / 2;
    for (size_t i = 0; i < durationCount && unitCount < 24; ++i) {
        if (durations[i] < shortest - shortest / 3) {
            continue;
        }
        if (durations[i] > sameClusterLimit) {
            break;
        }
        unitSum += durations[i];
        ++unitCount;
    }

    if (unitCount == 0) {
        return false;
    }

    const uint16_t unitUs = static_cast<uint16_t>(unitSum / unitCount);

    struct Candidate {
        bool decoded = false;
        bool inverted = false;
        uint8_t bitCount = 0;
        uint8_t unknownCount = 0;
        char bits[MaxDecodedBits + 1] = {};
    };

    auto decodeCandidate = [&](uint8_t activeLevel, bool inverted) {
        Candidate candidate;
        candidate.inverted = inverted;

        size_t i = 0;
        while (i < burstPulseCount_ && burst_[i].level != activeLevel) {
            ++i;
        }

        while (i + 1 < burstPulseCount_ && candidate.bitCount < MaxDecodedBits) {
            if (burst_[i].level != activeLevel || burst_[i + 1].level == activeLevel) {
                ++i;
                continue;
            }

            const char bit = classifyPair(burst_[i].durationUs, burst_[i + 1].durationUs, unitUs);
            candidate.bits[candidate.bitCount++] = bit;
            if (bit == '?') {
                ++candidate.unknownCount;
            }
            i += 2;
        }

        candidate.bits[candidate.bitCount] = '\0';
        candidate.decoded = candidate.bitCount >= 8 && candidate.unknownCount == 0;
        return candidate;
    };

    const Candidate normal = decodeCandidate(1, false);
    const Candidate inverted = decodeCandidate(0, true);
    const Candidate &best =
        (inverted.decoded && (!normal.decoded || inverted.unknownCount < normal.unknownCount))
            ? inverted
            : normal;

    if (!best.decoded) {
        result.unitUs = unitUs;
        return false;
    }

    result.decoded = true;
    result.inverted = best.inverted;
    result.bitCount = best.bitCount;
    result.unitUs = unitUs;
    strlcpy(result.bits, best.bits, sizeof(result.bits));
    bitsToHex(result.bits, result.bitCount, result.hex, sizeof(result.hex));
    return true;
}

void Cc1101Service::buildRawPreview(char *out, size_t outSize) const
{
    if (outSize == 0) {
        return;
    }

    size_t offset = 0;
    out[0] = '\0';
    for (size_t i = 0; i < burstPulseCount_ && offset + 1 < outSize; ++i) {
        const int written = snprintf(out + offset,
                                     outSize - offset,
                                     "%c%u%s",
                                     burst_[i].level ? 'H' : 'L',
                                     burst_[i].durationUs,
                                     i + 1 < burstPulseCount_ ? " " : "");
        if (written <= 0) {
            break;
        }

        if (static_cast<size_t>(written) >= outSize - offset) {
            out[outSize - 1] = '\0';
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
    next.recentBurst = lastBurstAtMs_ != 0 && now - lastBurstAtMs_ <= RecentBurstMs;
    next.lastDecoded = lastDecoded_;
    next.lastInverted = lastInverted_;
    next.lastInitAttemptAtMs = lastInitAttemptAtMs_;
    next.lastBurstAtMs = lastBurstAtMs_;
    next.burstCount = burstCount_;
    next.decodedBurstCount = decodedBurstCount_;
    next.rejectedBurstCount = rejectedBurstCount_;
    next.edgeOverflowCount = edgeOverflowCount_;
    next.lastState = lastState_;
    next.frequencyMhz = AppConfig::Cc1101FrequencyMhz;
    next.rssiDbm = lastRssiDbm_;
    next.noiseFloorDbm = noiseFloorDbm_;
    next.lastBurstRssiDbm = lastBurstRssiDbm_;
    next.lastPulseCount = lastPulseCount_;
    next.lastUnitUs = lastUnitUs_;
    next.lastBurstDurationMs = lastBurstDurationMs_;
    next.repeatCount = repeatCount_;
    next.lastBitCount = lastBitCount_;
    strlcpy(next.lastBits, lastBits_, sizeof(next.lastBits));
    strlcpy(next.lastHex, lastHex_, sizeof(next.lastHex));
    strlcpy(next.rawPreview, rawPreview_, sizeof(next.rawPreview));

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
    if (burstPulseCount_ == 0) {
        noiseFloorDbm_ = noiseFloorDbm_ * 0.95f + lastRssiDbm_ * 0.05f;
    } else if (lastRssiDbm_ > burstPeakRssiDbm_) {
        burstPeakRssiDbm_ = lastRssiDbm_;
    }
}

void Cc1101Service::restartReceive()
{
    if (captureAttached_) {
        detachInterrupt(digitalPinToInterrupt(AppConfig::Cc1101Gdo0Pin));
        captureAttached_ = false;
    }
    lastReceiveAttemptAtMs_ = millis();

    pinMode(AppConfig::Cc1101Gdo0Pin, INPUT);

    lastState_ = radio_.receiveDirectAsync();
    listening_ = lastState_ == RADIOLIB_ERR_NONE;
    if (!listening_) {
        Serial.printf("[cc1101] direct receive failed code=%d\n", lastState_);
        return;
    }

    portENTER_CRITICAL(&pulseMux_);
    pulseHead_ = 0;
    pulseTail_ = 0;
    pulseOverflowCount_ = 0;
    lastEdgeAtUs_ = micros();
    lastGdo0Level_ = static_cast<uint8_t>(digitalRead(AppConfig::Cc1101Gdo0Pin));
    portEXIT_CRITICAL(&pulseMux_);

    attachInterrupt(digitalPinToInterrupt(AppConfig::Cc1101Gdo0Pin), handleGdo0Change, CHANGE);
    captureAttached_ = true;
}

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void Cc1101Service::handleGdo0Change()
{
    const uint32_t now = micros();
    const uint8_t currentLevel = static_cast<uint8_t>(
        gpio_get_level(static_cast<gpio_num_t>(AppConfig::Cc1101Gdo0Pin)));

    portENTER_CRITICAL_ISR(&pulseMux_);

    const uint32_t elapsedUs = now - lastEdgeAtUs_;
    const uint16_t durationUs = elapsedUs > MaxStoredPulseUs
                                    ? MaxStoredPulseUs
                                    : static_cast<uint16_t>(elapsedUs);
    const uint16_t nextHead = (pulseHead_ + 1) % CapturedPulseBufferSize;

    if (nextHead == pulseTail_) {
        pulseOverflowCount_ = pulseOverflowCount_ + 1;
    } else {
        pulseDurations_[pulseHead_] = durationUs;
        pulseLevels_[pulseHead_] = lastGdo0Level_;
        pulseHead_ = nextHead;
    }

    lastEdgeAtUs_ = now;
    lastGdo0Level_ = currentLevel;

    portEXIT_CRITICAL_ISR(&pulseMux_);
}
