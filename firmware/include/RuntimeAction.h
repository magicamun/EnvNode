#pragma once

#include <stdint.h>

namespace EnvNode {

enum class RuntimeAction : uint8_t {
    None = 0,
    RestartMqtt,
    RestartTime,
    RestartWiFi,
    RestartSensorManager,
    RestartActuatorRuntime,
    RestartDevice,
};

const char* runtimeActionName(RuntimeAction action);

} // namespace EnvNode
