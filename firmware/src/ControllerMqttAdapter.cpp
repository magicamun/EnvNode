#include "ControllerMqttAdapter.h"

#include <climits>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
        commandSubscriptionFailureReported_ = false;
        parameterSubscriptionFailureReported_ = false;
        return;
    }
    const String& deviceName = configurationService_.getConfiguration().device.name;
    if (!commandSubscribed_) {
        const String topic = mqttControllerCommandSubscription(deviceName);
        commandSubscribed_ = mqttService_.subscribe(topic.c_str());
        if (commandSubscribed_) {
            if (commandSubscriptionFailureReported_) {
                logger_.info("MQTT Controller command subscription recovered");
            } else {
                logger_.debugf("MQTT Controller commands subscribed: %s", topic.c_str());
            }
            commandSubscriptionFailureReported_ = false;
        } else {
            if (!commandSubscriptionFailureReported_) {
                logger_.warnf(
                    "MQTT Controller command subscription failed: %s", topic.c_str());
                commandSubscriptionFailureReported_ = true;
            }
        }
    }
    if (!parameterSubscribed_) {
        const String topic = mqttControllerParameterCommandSubscription(deviceName);
        parameterSubscribed_ = mqttService_.subscribe(topic.c_str());
        if (parameterSubscribed_) {
            if (parameterSubscriptionFailureReported_) {
                logger_.info("MQTT Controller parameter subscription recovered");
            } else {
                logger_.debugf("MQTT Controller parameters subscribed: %s", topic.c_str());
            }
            parameterSubscriptionFailureReported_ = false;
        } else {
            if (!parameterSubscriptionFailureReported_) {
                logger_.warnf(
                    "MQTT Controller parameter subscription failed: %s", topic.c_str());
                parameterSubscriptionFailureReported_ = true;
            }
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
            logger_.warnf("MQTT Controller command rejected: slot %u is out of range",
                static_cast<unsigned int>(id));
            return;
        }
        handleCommand(id, payload, length);
        return;
    }
    ControllerParameter parameter = ControllerParameter::OnDurationMs;
    if (parseMqttControllerParameterCommandTopic(topic, deviceName, id, parameter)) {
        if (id > MaxControllerSlotCount) {
            logger_.warnf("MQTT Controller parameter rejected: slot %u is out of range",
                static_cast<unsigned int>(id));
            return;
        }
        handleParameter(id, parameter, payload, length);
        return;
    }
    const String prefix = mqttDeviceTopicRoot(deviceName) + "/controller/";
    if (topic != nullptr && strncmp(topic, prefix.c_str(), prefix.length()) == 0) {
        logger_.warnf("MQTT Controller message rejected: invalid topic %s", topic);
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
        logger_.warnf("MQTT Controller %u command rejected: expected START or STOP",
            static_cast<unsigned int>(id));
        return;
    }
    if (result == ControllerOperationResult::Completed
        || result == ControllerOperationResult::NoAction) {
        logger_.infof("MQTT Controller %u command accepted: %s",
            static_cast<unsigned int>(id), command);
    } else {
        logger_.warnf("MQTT Controller %u command failed: result=%u",
            static_cast<unsigned int>(id), static_cast<unsigned int>(result));
    }
}

void ControllerMqttAdapter::handleParameter(
    ControllerId id,
    ControllerParameter parameter,
    const uint8_t* payload,
    size_t length) {
    ControllerSlotConfiguration candidate =
        configurationService_.getConfiguration().controllerSlots[id - 1];
    String acceptedValue;
    if (candidate.implementation == ControllerImplementation::Blink
        && (parameter == ControllerParameter::OnDurationMs
            || parameter == ControllerParameter::OffDurationMs)) {
        uint32_t duration = 0;
        if (!parseDuration(payload, length, duration)) {
            logger_.warnf("MQTT Controller %u parameter rejected: invalid duration",
                static_cast<unsigned int>(id));
            return;
        }
        uint32_t& configured = parameter == ControllerParameter::OnDurationMs
            ? candidate.implementationConfiguration.blink.onDurationMs
            : candidate.implementationConfiguration.blink.offDurationMs;
        if (duration == configured) return;
        configured = duration;
        acceptedValue = String(duration);
    } else if (candidate.implementation == ControllerImplementation::Threshold
        && (parameter == ControllerParameter::OnThreshold
            || parameter == ControllerParameter::OffThreshold
            || parameter == ControllerParameter::ThresholdDirection
            || parameter == ControllerParameter::MaxMeasurementAgeMs)) {
        ThresholdControllerConfiguration& threshold =
            candidate.implementationConfiguration.threshold;
        if (parameter == ControllerParameter::ThresholdDirection) {
            ThresholdDirection requested;
            if (payload != nullptr && length == 8
                && memcmp(payload, "on_above", 8) == 0) {
                requested = ThresholdDirection::OnAbove;
                acceptedValue = "on_above";
            } else if (payload != nullptr && length == 8
                && memcmp(payload, "on_below", 8) == 0) {
                requested = ThresholdDirection::OnBelow;
                acceptedValue = "on_below";
            } else {
                logger_.warnf("MQTT Controller %u parameter rejected: expected on_above or on_below",
                    static_cast<unsigned int>(id));
                return;
            }
            if (requested == threshold.direction) return;
            threshold.direction = requested;
            const float previousOn = threshold.onThreshold;
            threshold.onThreshold = threshold.offThreshold;
            threshold.offThreshold = previousOn;
        } else if (parameter == ControllerParameter::MaxMeasurementAgeMs) {
            uint32_t age = 0;
            if (!parseUnsignedInteger(payload, length, age)) {
                logger_.warnf("MQTT Controller %u parameter rejected: invalid unsigned integer",
                    static_cast<unsigned int>(id));
                return;
            }
            if (age == threshold.maxMeasurementAgeMs) return;
            threshold.maxMeasurementAgeMs = age;
            acceptedValue = String(age);
        } else {
            float thresholdValue = 0.0F;
            if (!parseFiniteFloat(payload, length, thresholdValue)) {
                logger_.warnf("MQTT Controller %u parameter rejected: invalid finite decimal",
                    static_cast<unsigned int>(id));
                return;
            }
            float& configured = parameter == ControllerParameter::OnThreshold
                ? threshold.onThreshold : threshold.offThreshold;
            if (thresholdValue == configured) return;
            configured = thresholdValue;
            char formatted[24];
            snprintf(formatted, sizeof(formatted), "%.9g",
                static_cast<double>(thresholdValue));
            acceptedValue = String(formatted);
        }
    } else {
        logger_.warnf("MQTT Controller %u parameter rejected: unsupported for implementation",
            static_cast<unsigned int>(id));
        return;
    }
    if (!configurationService_.setControllerSlotConfiguration(candidate)) {
        logger_.warnf("MQTT Controller %u parameter rejected by configuration validation",
            static_cast<unsigned int>(id));
        return;
    }
    runtimeManager_.request(RuntimeAction::RestartControllerRuntime);
    if (!runtimeManager_.applyPendingControllerChanges(
            configurationService_.getConfiguration().controllerSlots)) {
        logger_.errorf("MQTT Controller %u parameter persisted but runtime apply failed",
            static_cast<unsigned int>(id));
        return;
    }
    logger_.infof("MQTT Controller %u parameter accepted: %s=%s",
        static_cast<unsigned int>(id), mqttControllerParameterName(parameter),
        acceptedValue.c_str());
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

bool ControllerMqttAdapter::parseUnsignedInteger(
    const uint8_t* payload,
    size_t length,
    uint32_t& value) {
    if (payload == nullptr || length == 0 || length > 10) return false;
    uint32_t parsed = 0;
    for (size_t index = 0; index < length; ++index) {
        if (payload[index] < '0' || payload[index] > '9') return false;
        const uint32_t digit = payload[index] - '0';
        if (parsed > (UINT32_MAX - digit) / 10U) return false;
        parsed = parsed * 10U + digit;
    }
    value = parsed;
    return true;
}

bool ControllerMqttAdapter::parseFiniteFloat(
    const uint8_t* payload,
    size_t length,
    float& value) {
    if (payload == nullptr || length == 0 || length > 47) return false;
    size_t index = 0;
    if (payload[index] == '+' || payload[index] == '-') ++index;
    bool hasDigit = false;
    while (index < length && payload[index] >= '0' && payload[index] <= '9') {
        hasDigit = true;
        ++index;
    }
    if (index < length && payload[index] == '.') {
        ++index;
        while (index < length && payload[index] >= '0' && payload[index] <= '9') {
            hasDigit = true;
            ++index;
        }
    }
    if (!hasDigit) return false;
    if (index < length && (payload[index] == 'e' || payload[index] == 'E')) {
        ++index;
        if (index < length && (payload[index] == '+' || payload[index] == '-')) ++index;
        const size_t exponentStart = index;
        while (index < length && payload[index] >= '0' && payload[index] <= '9') ++index;
        if (index == exponentStart) return false;
    }
    if (index != length) return false;
    char text[48];
    memcpy(text, payload, length);
    text[length] = '\0';
    char* end = nullptr;
    errno = 0;
    const float parsed = strtof(text, &end);
    if (end != text + length || errno == ERANGE || !std::isfinite(parsed)) return false;
    value = parsed;
    return true;
}

} // namespace EnvNode
