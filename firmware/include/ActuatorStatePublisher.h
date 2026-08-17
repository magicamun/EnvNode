#pragma once

#include "ActuatorRuntime.h"
#include "IConfigurationService.h"
#include "IMqttService.h"
#include "Logger.h"

namespace EnvNode {

class ActuatorStatePublisher {
public:
    ActuatorStatePublisher(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        ActuatorRuntime& actuatorRuntime);

    void loop();

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    ActuatorRuntime& actuatorRuntime_;
    bool wasConnected_ = false;
    bool known_[MaxActuatorSlotCount] = {};
    OnOffState states_[MaxActuatorSlotCount] = {};
    bool publicationFailureReported_[MaxActuatorSlotCount] = {};
};

} // namespace EnvNode
