#pragma once

#include <stdint.h>

namespace WeatherStation {

enum class RuntimeAction : uint8_t {
    None = 0,
    RestartMqtt,
    RestartTime,
    RestartWiFi,
    RestartSensorManager,
    RestartDevice,
};

const char* runtimeActionName(RuntimeAction action);

} // namespace WeatherStation
