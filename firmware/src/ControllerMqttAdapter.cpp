#include "ControllerMqttAdapter.h"

#include <climits>
#include <cstring>

namespace EnvNode {

ControllerMqttAdapter::ControllerMqttAdapter(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    ControllerRuntime& controllerRuntime,
    RuntimeManager& runtimeManager)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , controllerRuntime_(controllerRuntime)
    , runtimeManager_(runtimeManager) {
}

void ControllerMqttAdapter::begin() {
}

void ControllerMqttAdapter::loop() {
    if (!mqttService_.connected()) {
        commandSubscribed_ = false;
        parameterSubscribed_ = false;
        return;
    }
    const String& deviceName = configurationService_.getConfiguration().device.name;
    if (!commandSubscribed_) {
        const String topic = mqttControllerCommandSubscription(deviceName);
        commandSubscribed_ = mqttService_.subscribe(topic.c_str());
        if (commandSubscribed_) {
            logger_.printf("MQTT Controller commands subscribed: %s\n", topic.c_str());
        } else {
            logger_.printf("MQTT Controller command subscription failed: %s\n", topic.c_str());
        }
    }
    if (!parameterSubscribed_) {
        const String topic = mqttControllerParameterCommandSubscription(deviceName);
        parameterSubscribed_ = mqttService_.subscribe(topic.c_str());
        if (parameterSubscribed_) {
            logger_.printf("MQTT Controller parameters subscribed: %s\n", topic.c_str());
        } else {
            logger_.printf("MQTT Controller parameter subscription failed: %s\n", topic.c_str());
        }
    }
}

void ControllerMqttAdapter::handleMqttMessage(
    const char* topic,
    const uint8_t* payload,
    size_t length) {
    const String& deviceName = configurationService_.getConfiguration().device.name;
    ControllerId id = InvalidControllerId;
    if (parseMqttControllerCommandTopic(topic, deviceName, id)) {
        if (id > MaxControllerSlotCount) {
            logger_.printf("MQTT Controller command rejected: slot %u is out of range\n",
                static_cast<unsigned int>(id));
            return;
        }
        handleCommand(id, payload, length);
        return;
    }
    ControllerMqttParameter parameter = ControllerMqttParameter::OnDurationMs;
    if (parseMqttControllerParameterCommandTopic(topic, deviceName, id, parameter)) {
        if (id > MaxControllerSlotCount) {
            logger_.printf("MQTT Controller parameter rejected: slot %u is out of range\n",
                static_cast<unsigned int>(id));
            return;
        }
        handleParameter(id, parameter, payload, length);
        return;
    }
    const String prefix = mqttDeviceTopicRoot(deviceName) + "/controller/";
    if (topic != nullptr && strncmp(topic, prefix.c_str(), prefix.length()) == 0) {
        logger_.printf("MQTT Controller message rejected: invalid topic %s\n", topic);
    }
}

void ControllerMqttAdapter::handleCommand(
    ControllerId id,
    const uint8_t* payload,
    size_t length) {
    ControllerOperationResult result;
    const char* command = nullptr;
    if (payload != nullptr && length == 5 && memcmp(payload, "START", 5) == 0) {
        command = "START";
        result = controllerRuntime_.startController(id);
    } else if (payload != nullptr && length == 4 && memcmp(payload, "STOP", 4) == 0) {
        command = "STOP";
        result = controllerRuntime_.stopController(id);
    } else {
        logger_.printf("MQTT Controller %u command rejected: expected START or STOP\n",
            static_cast<unsigned int>(id));
        return;
    }
    if (result == ControllerOperationResult::Completed
        || result == ControllerOperationResult::NoAction) {
        logger_.printf("MQTT Controller %u command accepted: %s\n",
            static_cast<unsigned int>(id), command);
    } else {
        logger_.printf("MQTT Controller %u command failed: result=%u\n",
            static_cast<unsigned int>(id), static_cast<unsigned int>(result));
    }
}

void ControllerMqttAdapter::handleParameter(
    ControllerId id,
    ControllerMqttParameter parameter,
    const uint8_t* payload,
    size_t length) {
    uint32_t duration = 0;
    if (!parseDuration(payload, length, duration)) {
        logger_.printf("MQTT Controller %u parameter rejected: invalid duration\n",
            static_cast<unsigned int>(id));
        return;
    }
    ControllerSlotConfiguration candidate =
        configurationService_.getConfiguration().controllerSlots[id - 1];
    if (candidate.implementation != ControllerImplementation::Blink) {
        logger_.printf("MQTT Controller %u parameter rejected: implementation is not blink\n",
            static_cast<unsigned int>(id));
        return;
    }
    const uint32_t currentDuration = parameter == ControllerMqttParameter::OnDurationMs
        ? candidate.implementationConfiguration.blink.onDurationMs
        : candidate.implementationConfiguration.blink.offDurationMs;
    if (duration == currentDuration) {
        logger_.printf("MQTT Controller %u parameter unchanged: %s=%u\n",
            static_cast<unsigned int>(id), mqttControllerParameterName(parameter),
            static_cast<unsigned int>(duration));
        return;
    }
    if (parameter == ControllerMqttParameter::OnDurationMs) {
        candidate.implementationConfiguration.blink.onDurationMs = duration;
    } else {
        candidate.implementationConfiguration.blink.offDurationMs = duration;
    }
    if (!configurationService_.setControllerSlotConfiguration(candidate)) {
        logger_.printf("MQTT Controller %u parameter rejected by configuration validation\n",
            static_cast<unsigned int>(id));
        return;
    }
    runtimeManager_.request(RuntimeAction::RestartControllerRuntime);
    if (!runtimeManager_.applyPendingControllerChanges(
            configurationService_.getConfiguration().controllerSlots)) {
        logger_.printf("MQTT Controller %u parameter persisted but runtime apply failed\n",
            static_cast<unsigned int>(id));
        return;
    }
    logger_.printf("MQTT Controller %u parameter accepted: %s=%u\n",
        static_cast<unsigned int>(id), mqttControllerParameterName(parameter),
        static_cast<unsigned int>(duration));
}

bool ControllerMqttAdapter::parseDuration(
    const uint8_t* payload,
    size_t length,
    uint32_t& value) {
    if (payload == nullptr || length == 0 || length > 10) return false;
    uint32_t parsed = 0;
    for (size_t index = 0; index < length; ++index) {
        if (payload[index] < '0' || payload[index] > '9') return false;
        const uint32_t digit = payload[index] - '0';
        if (parsed > (static_cast<uint32_t>(INT32_MAX) - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
    }
    if (parsed == 0 || parsed > static_cast<uint32_t>(INT32_MAX)) return false;
    value = parsed;
    return true;
}

} // namespace EnvNode
