#pragma once

#include <Arduino.h>

#include "IConfigurationService.h"
#include "IMqttService.h"
#include "ActuatorRuntime.h"
#include "SensorManager.h"
#include "Logger.h"
#include "IDiscoveryPublisher.h"

namespace EnvNode {

class HomeAssistantDiscoveryPublisher : public IDiscoveryPublisher {
public:
    HomeAssistantDiscoveryPublisher(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        SensorManager& sensorManager,
        ActuatorRuntime& actuatorRuntime);

    void loop() override;
    DiscoveryRepublishResult republish() override;
    size_t lastPayloadSize() const;
    size_t lastEntityCount() const;

private:
    static constexpr uint32_t RetryIntervalMs = 10000;

    String stableDeviceId() const;
    String discoveryTopic() const;
    uint32_t discoverySignature(
        uint16_t* sensorComponentMasks,
        uint8_t* actuatorPlatforms) const;
    bool publishDiscovery(
        const uint16_t* sensorComponentMasks,
        const uint8_t* actuatorPlatforms,
        bool logPublication = true);
    bool publishPayload(const String& payload);
    String buildPayload(
        const uint16_t* sensorComponentMasks,
        const uint16_t* sensorRemovalMasks,
        const uint8_t* actuatorPlatforms,
        const uint8_t* actuatorRemovalPlatforms,
        size_t& entityCount) const;

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    SensorManager& sensorManager_;
    ActuatorRuntime& actuatorRuntime_;
    bool wasConnected_ = false;
    bool clearedAfterBoot_ = false;
    uint32_t publishedSignature_ = 0;
    uint32_t lastAttemptMs_ = 0;
    uint16_t publishedComponentMasks_[MaxSensorCount] = {};
    uint8_t publishedActuatorPlatforms_[MaxActuatorSlotCount] = {};
    size_t lastPayloadSize_ = 0;
    size_t lastEntityCount_ = 0;
    bool publicationFailureReported_ = false;
};

} // namespace EnvNode
