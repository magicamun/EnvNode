#include "MqttMessageRouter.h"

namespace EnvNode {

MqttMessageRouter::MqttMessageRouter(
    IMqttService& mqttService,
    IMqttMessageHandler& first,
    IMqttMessageHandler& second, IMqttMessageHandler* third)
    : mqttService_(mqttService), first_(first), second_(second), third_(third) {
}

void MqttMessageRouter::begin() {
    mqttService_.setMessageHandler(this);
}

void MqttMessageRouter::handleMqttMessage(
    const char* topic,
    const uint8_t* payload,
    size_t length) {
    first_.handleMqttMessage(topic, payload, length);
    second_.handleMqttMessage(topic, payload, length);
    if (third_) third_->handleMqttMessage(topic, payload, length);
}

} // namespace EnvNode
