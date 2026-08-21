#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareResources.h"

namespace EnvNode {

enum class ActuatorImplementation : uint8_t {
    None = 0,
    GpioOnOff = 1,
    GpioPwm = 2,
};

enum class ActuatorCapability : uint8_t {
    None = 0,
    OnOff = 1U << 0,
    Level = 1U << 1,
};

constexpr ActuatorCapability effectiveActuatorCapabilities(
    ActuatorCapability capabilities) {
    return (static_cast<uint8_t>(capabilities)
            & static_cast<uint8_t>(ActuatorCapability::Level)) != 0
        ? static_cast<ActuatorCapability>(
            static_cast<uint8_t>(capabilities)
            | static_cast<uint8_t>(ActuatorCapability::OnOff))
        : capabilities;
}

constexpr bool hasActuatorCapability(
    ActuatorCapability available,
    ActuatorCapability required) {
    return (static_cast<uint8_t>(effectiveActuatorCapabilities(available))
            & static_cast<uint8_t>(required))
        == static_cast<uint8_t>(required);
}

struct ActuatorImplementationMetadata {
    ActuatorImplementation implementation;
    const char* stableId;
    const char* displayType;
    ActuatorCapability capabilities;
    HardwareInterfaceKind interfaceKind;
    GpioCapability requiredGpioCapabilities;
    const char* protocolDescription;
};

class ActuatorImplementationRegistry {
public:
    static size_t count();
    static const ActuatorImplementationMetadata* at(size_t index);
    static const ActuatorImplementationMetadata* find(ActuatorImplementation implementation);
    static const ActuatorImplementationMetadata* findByStableId(const char* stableId);
};

} // namespace EnvNode
