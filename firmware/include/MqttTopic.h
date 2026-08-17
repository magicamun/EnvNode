#pragma once

#include <Arduino.h>

#include "MeasurementType.h"
#include "ActuatorId.h"
#include "ControllerId.h"

namespace EnvNode {

const char* mqttTopicRoot();
String mqttTopicSafeDeviceName(const String& deviceName);
String mqttDeviceTopicRoot(const String& deviceName);
const char* mqttMeasurementTypeTopic(MeasurementType type);
String mqttActuatorCommandSubscription(const String& deviceName);
String mqttActuatorCommandTopic(const String& deviceName, ActuatorId id);
String mqttActuatorStatusTopic(const String& deviceName, ActuatorId id);
bool parseMqttActuatorCommandTopic(
    const char* topic,
    const String& deviceName,
    ActuatorId& id);

enum class ControllerMqttParameter : uint8_t {
    OnDurationMs,
    OffDurationMs,
    OnThreshold,
    OffThreshold,
    MaxMeasurementAgeMs,
    Count,
};

const char* mqttControllerParameterName(ControllerMqttParameter parameter);
String mqttControllerCommandSubscription(const String& deviceName);
String mqttControllerParameterCommandSubscription(const String& deviceName);
String mqttControllerCommandTopic(const String& deviceName, ControllerId id);
String mqttControllerStatusTopic(const String& deviceName, ControllerId id);
String mqttControllerParameterTopic(
    const String& deviceName,
    ControllerId id,
    ControllerMqttParameter parameter);
String mqttControllerParameterCommandTopic(
    const String& deviceName,
    ControllerId id,
    ControllerMqttParameter parameter);
bool parseMqttControllerCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id);
bool parseMqttControllerParameterCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id,
    ControllerMqttParameter& parameter);

} // namespace EnvNode
