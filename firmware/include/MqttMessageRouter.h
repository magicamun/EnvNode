#pragma once

#include "IMqttService.h"

namespace EnvNode {

class MqttMessageRouter : public IMqttMessageHandler {
public:
    MqttMessageRouter(
        IMqttService& mqttService,
        IMqttMessageHandler& first,
        IMqttMessageHandler& second, IMqttMessageHandler* third = nullptr);

    void begin();
    void handleMqttMessage(
        const char* topic,
        const uint8_t* payload,
        size_t length) override;

private:
    IMqttService& mqttService_;
    IMqttMessageHandler& first_;
    IMqttMessageHandler& second_;
    IMqttMessageHandler* third_;
};

} // namespace EnvNode
