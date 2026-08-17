#pragma once

#include <cstdint>

namespace EnvNode {

enum class OnOffState : uint8_t {
    Off = 0,
    On = 1,
};

enum class ActuatorOperationResult : uint8_t {
    Completed,
    InvalidHardwareResource,
    NotInitialized,
};

class IOnOffActuator {
public:
    virtual ~IOnOffActuator() = default;

    virtual ActuatorOperationResult begin() = 0;
    virtual ActuatorOperationResult shutdown() = 0;
    virtual ActuatorOperationResult setState(OnOffState state) = 0;
    virtual OnOffState state() const = 0;
    virtual bool initialized() const = 0;
};

} // namespace EnvNode
