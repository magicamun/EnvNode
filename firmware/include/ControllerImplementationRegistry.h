#pragma once

#include <cstddef>
#include <cstdint>

#include "ActuatorImplementationRegistry.h"

namespace EnvNode {

enum class ControllerImplementation : uint8_t {
    None = 0,
    Blink = 1,
    Threshold = 2,
    Selector = 3,
};

struct ControllerImplementationMetadata {
    ControllerImplementation implementation;
    const char* stableId;
    const char* displayType;
    ActuatorCapability requiredActuatorCapabilities;
    const char* description;
};

class ControllerImplementationRegistry {
public:
    static size_t count();
    static const ControllerImplementationMetadata* at(size_t index);
    static const ControllerImplementationMetadata* find(
        ControllerImplementation implementation);
    static const ControllerImplementationMetadata* findByStableId(
        const char* stableId);
};

} // namespace EnvNode
