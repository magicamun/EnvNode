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
}

void ActuatorMqttAdapter::loop() {
    const bool connected = mqttService_.connected();
    if (!connected) {
        onOffSubscribed_ = false;
        levelSubscribed_ = false;
        onOffSubscriptionFailureReported_ = false;
        levelSubscriptionFailureReported_ = false;
        return;
    }
    if (!onOffSubscribed_) {
        const String topic = mqttActuatorCommandSubscription(
            configurationService_.getConfiguration().device.name);
        if (mqttService_.subscribe(topic.c_str())) {
            onOffSubscribed_ = true;
            if (onOffSubscriptionFailureReported_) {
                logger_.info("MQTT actuator command subscription recovered");
            } else {
                logger_.debugf("MQTT actuator commands subscribed: %s", topic.c_str());
            }
            onOffSubscriptionFailureReported_ = false;
        } else {
            if (!onOffSubscriptionFailureReported_) {
                logger_.warnf(
                    "MQTT actuator command subscription failed: %s", topic.c_str());
                onOffSubscriptionFailureReported_ = true;
            }
        }
    }
    if (!levelSubscribed_) {
        const String topic = mqttActuatorLevelCommandSubscription(
            configurationService_.getConfiguration().device.name);
        if (mqttService_.subscribe(topic.c_str())) {
            levelSubscribed_ = true;
            if (levelSubscriptionFailureReported_) {
                logger_.info("MQTT actuator Level command subscription recovered");
            } else {
                logger_.debugf("MQTT actuator Level commands subscribed: %s", topic.c_str());
            }
            levelSubscriptionFailureReported_ = false;
        } else if (!levelSubscriptionFailureReported_) {
            logger_.warnf("MQTT actuator Level command subscription failed: %s",
                topic.c_str());
            levelSubscriptionFailureReported_ = true;
        }
    }
}

void ActuatorMqttAdapter::handleMqttMessage(
    const char* topic,
    const uint8_t* payload,
    size_t length) {
    ActuatorId id = InvalidActuatorId;
    const bool onOffCommand = parseMqttActuatorCommandTopic(
            topic,
            configurationService_.getConfiguration().device.name,
            id);
    const bool levelCommand = !onOffCommand && parseMqttActuatorLevelCommandTopic(
            topic,
            configurationService_.getConfiguration().device.name,
            id);
    if (!onOffCommand && !levelCommand) {
        return;
    }
    if (id > MaxActuatorSlotCount) {
        logger_.warnf("MQTT actuator command rejected: invalid topic %s",
            topic == nullptr ? "(null)" : topic);
        return;
    }

    ActuatorOperationResult result = ActuatorOperationResult::NotInitialized;
    if (onOffCommand) {
        OnOffState requestedState;
        if (length == 2 && payload != nullptr && memcmp(payload, "ON", 2) == 0) {
            requestedState = OnOffState::On;
        } else if (length == 3 && payload != nullptr && memcmp(payload, "OFF", 3) == 0) {
            requestedState = OnOffState::Off;
        } else {
            logger_.warnf("MQTT actuator %u command rejected: invalid on_off payload",
                static_cast<unsigned int>(id));
            return;
        }
        IOnOffActuator* actuator = actuatorRuntime_.onOffActuator(id);
        if (actuator == nullptr) {
            logger_.warnf("MQTT actuator %u command rejected: OnOff capability unavailable",
                static_cast<unsigned int>(id));
            return;
        }
        result = actuator->setState(requestedState);
    } else {
        if (payload == nullptr || length == 0 || length > 3) {
            logger_.warnf("MQTT actuator %u command rejected: invalid Level payload",
                static_cast<unsigned int>(id));
            return;
        }
        uint16_t percentage = 0;
        for (size_t index = 0; index < length; ++index) {
            if (payload[index] < '0' || payload[index] > '9') {
                logger_.warnf("MQTT actuator %u command rejected: invalid Level payload",
                    static_cast<unsigned int>(id));
                return;
            }
            percentage = static_cast<uint16_t>(percentage * 10 + payload[index] - '0');
        }
        ActuatorLevel requestedLevel = ActuatorLevel::off();
        if (percentage > 255
            || !ActuatorLevel::tryCreate(static_cast<uint8_t>(percentage), requestedLevel)) {
            logger_.warnf("MQTT actuator %u command rejected: Level outside 0..100",
                static_cast<unsigned int>(id));
            return;
        }
        ILevelActuator* actuator = actuatorRuntime_.levelActuator(id);
        if (actuator == nullptr) {
            logger_.warnf("MQTT actuator %u command rejected: Level capability unavailable",
                static_cast<unsigned int>(id));
            return;
        }
        result = actuator->setLevel(requestedLevel);
    }
    if (result != ActuatorOperationResult::Completed) {
        logger_.warnf("MQTT actuator %u command failed: result=%u",
            static_cast<unsigned int>(id),
            static_cast<unsigned int>(result));
    }
}

} // namespace EnvNode
