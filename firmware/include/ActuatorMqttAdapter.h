#pragma once

#include "ActuatorRuntime.h"
#include "IConfigurationService.h"
#include "IMqttService.h"
#include "Logger.h"

namespace EnvNode {

class ActuatorMqttAdapter : public IMqttMessageHandler {
public:
    ActuatorMqttAdapter(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        ActuatorRuntime& actuatorRuntime);

    void begin();
    void loop();
    void handleMqttMessage(
        const char* topic,
        const uint8_t* payload,
        size_t length) override;

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    ActuatorRuntime& actuatorRuntime_;
    bool onOffSubscribed_ = false;
    bool levelSubscribed_ = false;
    bool onOffSubscriptionFailureReported_ = false;
    bool levelSubscriptionFailureReported_ = false;
};

} // namespace EnvNode
