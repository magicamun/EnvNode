#pragma once

#include <Arduino.h>

#include "MeasurementType.h"

namespace WeatherStation {

String mqttTopicSafeDeviceName(const String& deviceName);
const char* mqttMeasurementTypeTopic(MeasurementType type);

} // namespace WeatherStation
