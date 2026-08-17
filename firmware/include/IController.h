#pragma once

#include <cstdint>

namespace EnvNode {

enum class ControllerOperationResult : uint8_t {
    Completed,
    NoAction,
    TargetUnavailable,
    ActuatorOperationFailed,
    InvalidConfiguration,
    NotRunning,
    ControllerNotFound,
};

class IController {
public:
    virtual ~IController() = default;

    virtual ControllerOperationResult begin() = 0;
    virtual ControllerOperationResult service() = 0;
    virtual ControllerOperationResult stop() = 0;
};

} // namespace EnvNode
