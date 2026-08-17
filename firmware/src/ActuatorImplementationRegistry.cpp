#include "ActuatorImplementationRegistry.h"

#include <cstring>

namespace EnvNode {
namespace {

constexpr size_t ImplementationCount = 2;

const ActuatorImplementationMetadata* implementations() {
    static const ActuatorImplementationMetadata registeredImplementations[ImplementationCount] = {
        {ActuatorImplementation::None, "none", "None", ActuatorCapability::None,
            HardwareInterfaceKind::Simulation, GpioCapability::None, "No runtime actuator"},
        {ActuatorImplementation::GpioOnOff, "gpio_on_off", "GPIO On/Off",
            ActuatorCapability::OnOff, HardwareInterfaceKind::GPIO,
            GpioCapability::DigitalOutput, "Digital output"},
    };
    return registeredImplementations;
}

} // namespace

size_t ActuatorImplementationRegistry::count() {
    return ImplementationCount;
}

const ActuatorImplementationMetadata* ActuatorImplementationRegistry::at(size_t index) {
    return index < count() ? &implementations()[index] : nullptr;
}

const ActuatorImplementationMetadata* ActuatorImplementationRegistry::find(
    ActuatorImplementation implementation) {
    const ActuatorImplementationMetadata* registeredImplementations = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (registeredImplementations[index].implementation == implementation) {
            return &registeredImplementations[index];
        }
    }
    return nullptr;
}

const ActuatorImplementationMetadata* ActuatorImplementationRegistry::findByStableId(
    const char* stableId) {
    if (stableId == nullptr) return nullptr;
    const ActuatorImplementationMetadata* registeredImplementations = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (strcmp(registeredImplementations[index].stableId, stableId) == 0) {
            return &registeredImplementations[index];
        }
    }
    return nullptr;
}

} // namespace EnvNode
