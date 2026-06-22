#include "Cc1101Service.h"

#include <SPI.h>
#include <string.h>

#include "AppConfig.h"

volatile bool Cc1101Service::packetReceived_ = false;

namespace {

constexpr size_t PacketBufferSize = 255;
constexpr uint32_t RecentPacketMs = 2000;

void formatHexPreview(const uint8_t *data, size_t length, char *out, size_t outSize)
{
    static constexpr char HexDigits[] = "0123456789ABCDEF";

    if (outSize == 0) {
        return;
    }

    const size_t maxBytes = (outSize - 1) / 2;
    const size_t bytesToWrite = length < maxBytes ? length : maxBytes;
    for (size_t i = 0; i < bytesToWrite; ++i) {
        out[i * 2] = HexDigits[(data[i] >> 4) & 0x0F];
        out[i * 2 + 1] = HexDigits[data[i] & 0x0F];
    }
    out[bytesToWrite * 2] = '\0';
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

    if (packetReceived_) {
        packetReceived_ = false;
        handleReceivedPacket();
        restartReceive();
        publishSnapshot();
        return;
    }

    refreshRssi();
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
    lastInitAttemptAtMs_ = millis();
    listening_ = false;
    packetReceived_ = false;

    ConfigFSK_t config;
    config.frequency = AppConfig::Cc1101FrequencyMhz;
    config.bitRate = AppConfig::Cc1101BitRateKbps;
    config.frequencyDeviation = AppConfig::Cc1101FrequencyDeviationKhz;
    config.receiverBandwidth = AppConfig::Cc1101RxBandwidthKhz;
    config.power = AppConfig::Cc1101OutputPowerDbm;
    config.preambleLength = AppConfig::Cc1101PreambleLengthBits;

    lastState_ = radio_.begin(config);
    radioReady_ = lastState_ == RADIOLIB_ERR_NONE;

    if (!radioReady_) {
        Serial.printf("[cc1101] init failed code=%d\n", lastState_);
        return false;
    }

    radio_.setPacketReceivedAction(handlePacketReceived);
    restartReceive();
    Serial.printf("[cc1101] listening %.2f MHz, br=%.1f kbps, rx_bw=%.1f kHz\n",
                  AppConfig::Cc1101FrequencyMhz,
                  AppConfig::Cc1101BitRateKbps,
                  AppConfig::Cc1101RxBandwidthKhz);
    return true;
}

void Cc1101Service::handleReceivedPacket()
{
    uint8_t packet[PacketBufferSize] = {};
    const size_t reportedLength = radio_.getPacketLength();
    const size_t readLength = reportedLength < sizeof(packet) ? reportedLength : sizeof(packet);

    lastState_ = radio_.readData(packet, readLength);
    if (lastState_ == RADIOLIB_ERR_NONE) {
        ++packetCount_;
        lastPacketAtMs_ = millis();
        lastRssiDbm_ = radio_.getRSSI();
        lastLqi_ = radio_.getLQI();
        lastPacketLength_ = static_cast<uint8_t>(
            reportedLength > 255 ? 255 : reportedLength);
        formatHexPreview(packet, readLength, lastPacketHex_, sizeof(lastPacketHex_));

        Serial.printf("[cc1101] packet count=%lu len=%u rssi=%.1fdBm lqi=%u data=%s\n",
                      static_cast<unsigned long>(packetCount_),
                      lastPacketLength_,
                      lastRssiDbm_,
                      lastLqi_,
                      lastPacketHex_);
        return;
    }

    if (lastState_ == RADIOLIB_ERR_CRC_MISMATCH) {
        ++crcErrorCount_;
        Serial.println("[cc1101] crc error");
        return;
    }

    ++receiveErrorCount_;
    Serial.printf("[cc1101] receive failed code=%d\n", lastState_);
}

void Cc1101Service::publishSnapshot()
{
    Cc1101Snapshot next;
    const uint32_t now = millis();

    next.uptimeMs = now;
    next.radioReady = radioReady_;
    next.listening = listening_;
    next.recentPacket = lastPacketAtMs_ != 0 && now - lastPacketAtMs_ <= RecentPacketMs;
    next.lastInitAttemptAtMs = lastInitAttemptAtMs_;
    next.lastPacketAtMs = lastPacketAtMs_;
    next.packetCount = packetCount_;
    next.crcErrorCount = crcErrorCount_;
    next.receiveErrorCount = receiveErrorCount_;
    next.lastState = lastState_;
    next.frequencyMhz = AppConfig::Cc1101FrequencyMhz;
    next.rssiDbm = lastRssiDbm_;
    next.lqi = lastLqi_;
    next.lastPacketLength = lastPacketLength_;
    strlcpy(next.lastPacketHex, lastPacketHex_, sizeof(next.lastPacketHex));

    if (xSemaphoreTake(snapshotMutex_, pdMS_TO_TICKS(20)) == pdTRUE) {
        snapshot_ = next;
        xSemaphoreGive(snapshotMutex_);
    }
}

void Cc1101Service::refreshRssi()
{
    if (radioReady_) {
        lastRssiDbm_ = radio_.getRSSI();
    }
}

void Cc1101Service::restartReceive()
{
    lastReceiveAttemptAtMs_ = millis();
    lastState_ = radio_.startReceive();
    listening_ = lastState_ == RADIOLIB_ERR_NONE;
    if (!listening_) {
        Serial.printf("[cc1101] listen failed code=%d\n", lastState_);
    }
}

#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void Cc1101Service::handlePacketReceived()
{
    packetReceived_ = true;
}
