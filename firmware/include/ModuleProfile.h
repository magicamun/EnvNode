#pragma once

#include <cstddef>
#include <cstdint>

#include "HardwareResources.h"
#include "ModuleIdentity.h"

namespace EnvNode {

enum class ModuleCapability : uint8_t {
    None = 0,
    OnOffOutputs = 1U << 0,
    AnalogCurrentInput = 1U << 1,
    LocalProbeSupply = 1U << 2,
};

constexpr ModuleCapability operator|(ModuleCapability left, ModuleCapability right) {
    return static_cast<ModuleCapability>(
        static_cast<uint8_t>(left) | static_cast<uint8_t>(right));
}

constexpr bool hasModuleCapability(
    ModuleCapability available,
    ModuleCapability required) {
    return (static_cast<uint8_t>(available) & static_cast<uint8_t>(required))
        == static_cast<uint8_t>(required);
}

enum class ModuleConnectorResource : uint8_t {
    I2C0,
    I2C1,
    AuxGpio1,
    AuxGpio2,
    SPI,
    FiveVoltSupply,
};

struct ModuleResourceRequirement {
    ModuleConnectorResource resource;
    GpioCapability requiredGpioCapabilities;
};

constexpr size_t MaximumModuleResourceRequirementCount = 4;

struct ModuleProfile {
    ModuleProfileId id;
    const char* stableId;
    const char* displayName;
    ModuleRevision revision;
    ModuleCapability capabilities;
    ModuleResourceRequirement requirements[MaximumModuleResourceRequirementCount];
    size_t requirementCount;
};

class ModuleProfileRegistry {
public:
    // Optional catalog for interpreting known legacy EMID v1 profile IDs.
    // Future self-describing descriptors do not require a catalog entry.
    static size_t count();
    static const ModuleProfile* at(size_t index);
    static const ModuleProfile* find(ModuleProfileId id, ModuleRevision revision);
    static const ModuleProfile* findByStableId(const char* stableId);
};

const char* moduleConnectorResourceName(ModuleConnectorResource resource);

} // namespace EnvNode
