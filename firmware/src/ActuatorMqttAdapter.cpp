#include "ActuatorMqttAdapter.h"

#include <cstring>

#include "MqttTopic.h"

namespace EnvNode {

ActuatorMqttAdapter::ActuatorMqttAdapter(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    ActuatorRuntime& actuatorRuntime)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , actuatorRuntime_(actuatorRuntime) {
}

void ActuatorMqttAdapter::begin() {
    mqttService_.setMessageHandler(this);
}

void ActuatorMqttAdapter::loop() {
    const bool connected = mqttService_.connected();
    if (!connected) {
        subscribed_ = false;
        return;
    }
    if (!subscribed_) {
        const String topic = mqttActuatorCommandSubscription(
            configurationService_.getConfiguration().device.name);
        if (mqttService_.subscribe(topic.c_str())) {
            subscribed_ = true;
            logger_.printf("MQTT actuator commands subscribed: %s\n", topic.c_str());
        } else {
            logger_.printf("MQTT actuator command subscription failed: %s\n", topic.c_str());
        }
    }
}

void ActuatorMqttAdapter::handleMqttMessage(
    const char* topic,
    const uint8_t* payload,
    size_t length) {
    ActuatorId id = InvalidActuatorId;
    if (!parseMqttActuatorCommandTopic(
            topic,
            configurationService_.getConfiguration().device.name,
            id)
        || id > MaxActuatorSlotCount) {
        logger_.printf("MQTT actuator command rejected: invalid topic %s\n",
            topic == nullptr ? "(null)" : topic);
        return;
    }

    OnOffState requestedState;
    if (length == 2 && payload != nullptr && memcmp(payload, "ON", 2) == 0) {
        requestedState = OnOffState::On;
    } else if (length == 3 && payload != nullptr && memcmp(payload, "OFF", 3) == 0) {
        requestedState = OnOffState::Off;
    } else {
        logger_.printf("MQTT actuator %u command rejected: invalid on_off payload\n",
            static_cast<unsigned int>(id));
        return;
    }

    IOnOffActuator* actuator = actuatorRuntime_.onOffActuator(id);
    if (actuator == nullptr) {
        logger_.printf("MQTT actuator %u command rejected: OnOff capability unavailable\n",
            static_cast<unsigned int>(id));
        return;
    }
    const ActuatorOperationResult result = actuator->setState(requestedState);
    if (result != ActuatorOperationResult::Completed) {
        logger_.printf("MQTT actuator %u command failed: result=%u\n",
            static_cast<unsigned int>(id),
            static_cast<unsigned int>(result));
    }
}

} // namespace EnvNode
