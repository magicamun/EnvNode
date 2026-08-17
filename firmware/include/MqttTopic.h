#pragma once

#include <Arduino.h>

#include "MeasurementType.h"
#include "ActuatorId.h"

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

} // namespace EnvNode
