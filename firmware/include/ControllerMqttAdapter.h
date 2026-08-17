#pragma once

#include "ControllerRuntime.h"
#include "IConfigurationService.h"
#include "IMqttService.h"
#include "Logger.h"
#include "MqttTopic.h"
#include "RuntimeManager.h"

namespace EnvNode {

class ControllerMqttAdapter : public IMqttMessageHandler {
public:
    ControllerMqttAdapter(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        ControllerRuntime& controllerRuntime,
        RuntimeManager& runtimeManager);

    void begin();
    void loop();
    void handleMqttMessage(
        const char* topic,
        const uint8_t* payload,
        size_t length) override;

private:
    void handleCommand(ControllerId id, const uint8_t* payload, size_t length);
    void handleParameter(
        ControllerId id,
        ControllerParameter parameter,
        const uint8_t* payload,
        size_t length);
    static bool parseDuration(const uint8_t* payload, size_t length, uint32_t& value);
    static bool parseUnsignedInteger(
        const uint8_t* payload,
        size_t length,
        uint32_t& value);
    static bool parseFiniteFloat(const uint8_t* payload, size_t length, float& value);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    ControllerRuntime& controllerRuntime_;
    RuntimeManager& runtimeManager_;
    bool commandSubscribed_ = false;
    bool parameterSubscribed_ = false;
};

} // namespace EnvNode
