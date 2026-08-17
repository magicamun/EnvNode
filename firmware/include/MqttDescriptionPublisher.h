#pragma once

#include <cstdint>

#include "IConfigurationService.h"
#include "IMonotonicClock.h"
#include "IMqttService.h"
#include "Logger.h"

namespace EnvNode {

class MqttDescriptionPublisher {
public:
    MqttDescriptionPublisher(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        IMonotonicClock& clock);

    void loop();

private:
    static constexpr uint32_t RetryDelayMs = 1000;

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    IMonotonicClock& clock_;
    bool wasConnected_ = false;
    bool reconciliationActive_ = false;
    bool failureReported_ = false;
    uint32_t retryAtMs_ = 0;
    bool actuatorSynchronized_[MaxActuatorSlotCount] = {};
    uint32_t actuatorSignatures_[MaxActuatorSlotCount] = {};
    bool controllerSynchronized_[MaxControllerSlotCount] = {};
    uint32_t controllerSignatures_[MaxControllerSlotCount] = {};

    void beginReconciliation();
    bool synchronizeActuator(size_t index, const Configuration& configuration);
    bool synchronizeController(size_t index, const Configuration& configuration);
};

} // namespace EnvNode
