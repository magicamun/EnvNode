#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareResources.h"

namespace EnvNode {

enum class ActuatorImplementation : uint8_t {
    None = 0,
    GpioOnOff = 1,
};

enum class ActuatorCapability : uint8_t {
    None = 0,
    OnOff = 1U << 0,
};

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
