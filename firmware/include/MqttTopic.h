#pragma once

#include <Arduino.h>

#include "MeasurementType.h"
#include "SensorId.h"
#include "ActuatorId.h"
#include "ControllerId.h"
#include "ControllerParameterMetadata.h"

namespace EnvNode {

const char* mqttTopicRoot();
String mqttTopicSafeDeviceName(const String& deviceName);
String mqttDeviceTopicRoot(const String& deviceName);
const char* mqttMeasurementTypeTopic(MeasurementType type);
String mqttMeasurementTopic(
    const String& deviceName,
    SensorId sensorId,
    MeasurementType type);
String mqttActuatorCommandSubscription(const String& deviceName);
String mqttActuatorCommandTopic(const String& deviceName, ActuatorId id);
String mqttActuatorStatusTopic(const String& deviceName, ActuatorId id);
String mqttActuatorDescriptionTopic(const String& deviceName, ActuatorId id);
bool parseMqttActuatorCommandTopic(
    const char* topic,
    const String& deviceName,
    ActuatorId& id);

const char* mqttControllerParameterName(ControllerParameter parameter);
String mqttControllerCommandSubscription(const String& deviceName);
String mqttControllerParameterCommandSubscription(const String& deviceName);
String mqttControllerCommandTopic(const String& deviceName, ControllerId id);
String mqttControllerStatusTopic(const String& deviceName, ControllerId id);
String mqttControllerDescriptionTopic(const String& deviceName, ControllerId id);
String mqttControllerParameterTopic(
    const String& deviceName,
    ControllerId id,
    ControllerParameter parameter);
String mqttControllerParameterCommandTopic(
    const String& deviceName,
    ControllerId id,
    ControllerParameter parameter);
bool parseMqttControllerCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id);
bool parseMqttControllerParameterCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id,
    ControllerParameter& parameter);

} // namespace EnvNode
