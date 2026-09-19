#include "ModuleProfile.h"

#include <cstring>

namespace EnvNode {
namespace {

constexpr size_t ProfileCount = 2;

const ModuleProfile* profiles() {
    static const ModuleProfile registeredProfiles[ProfileCount] = {
        {
            ModuleProfileId::DuoRelay,
            "duo_relay",
            "DuoRelay",
            {0, 3},
            ModuleCapability::OnOffOutputs,
            {
                {ModuleConnectorResource::AuxGpio1, GpioCapability::DigitalOutput},
                {ModuleConnectorResource::AuxGpio2, GpioCapability::DigitalOutput},
                {ModuleConnectorResource::FiveVoltSupply, GpioCapability::None},
                {},
            },
            3,
        },
        {
            ModuleProfileId::AnalogHydroPressure,
            "analog_hydro_pressure",
            "AnalogHydroPressure",
            {0, 3},
            ModuleCapability::AnalogCurrentInput | ModuleCapability::LocalProbeSupply,
            {
                {ModuleConnectorResource::I2C0, GpioCapability::None},
                {ModuleConnectorResource::FiveVoltSupply, GpioCapability::None},
                {},
                {},
            },
            2,
        },
    };
    return registeredProfiles;
}

bool revisionsEqual(ModuleRevision left, ModuleRevision right) {
    return left.major == right.major && left.minor == right.minor;
}

} // namespace

size_t ModuleProfileRegistry::count() {
    return ProfileCount;
}

const ModuleProfile* ModuleProfileRegistry::at(size_t index) {
    return index < count() ? &profiles()[index] : nullptr;
}

const ModuleProfile* ModuleProfileRegistry::find(
    ModuleProfileId id,
    ModuleRevision revision) {
    const ModuleProfile* registeredProfiles = profiles();
    for (size_t index = 0; index < count(); ++index) {
        if (registeredProfiles[index].id == id
            && revisionsEqual(registeredProfiles[index].revision, revision)) {
            return &registeredProfiles[index];
        }
    }
    return nullptr;
}

const ModuleProfile* ModuleProfileRegistry::findByStableId(const char* stableId) {
    if (stableId == nullptr) return nullptr;
    const ModuleProfile* registeredProfiles = profiles();
    for (size_t index = 0; index < count(); ++index) {
        if (std::strcmp(registeredProfiles[index].stableId, stableId) == 0) {
            return &registeredProfiles[index];
        }
    }
    return nullptr;
}

const char* moduleConnectorResourceName(ModuleConnectorResource resource) {
    switch (resource) {
        case ModuleConnectorResource::I2C0: return "I2C0";
        case ModuleConnectorResource::I2C1: return "I2C1";
        case ModuleConnectorResource::AuxGpio1: return "AUX_GPIO1";
        case ModuleConnectorResource::AuxGpio2: return "AUX_GPIO2";
        case ModuleConnectorResource::SPI: return "SPI";
        case ModuleConnectorResource::FiveVoltSupply: return "+5V";
        default: return "unknown";
    }
}

} // namespace EnvNode
