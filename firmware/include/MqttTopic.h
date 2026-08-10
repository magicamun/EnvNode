#pragma once

#include <Arduino.h>

#include "MeasurementType.h"

namespace EnvNode {

const char* mqttTopicRoot();
String mqttTopicSafeDeviceName(const String& deviceName);
String mqttDeviceTopicRoot(const String& deviceName);
const char* mqttMeasurementTypeTopic(MeasurementType type);

} // namespace EnvNode
