#include "ControllerImplementationRegistry.h"

#include <cstring>

namespace EnvNode {
namespace {

constexpr size_t ImplementationCount = 2;

const ControllerImplementationMetadata* implementations() {
    static const ControllerImplementationMetadata registered[ImplementationCount] = {
        {ControllerImplementation::None, "none", "None", ActuatorCapability::None,
            "No runtime controller"},
        {ControllerImplementation::Blink, "blink", "Blink", ActuatorCapability::OnOff,
            "Periodically switches an On/Off actuator"},
    };
    return registered;
}

} // namespace

size_t ControllerImplementationRegistry::count() {
    return ImplementationCount;
}

const ControllerImplementationMetadata* ControllerImplementationRegistry::at(
    size_t index) {
    return index < count() ? &implementations()[index] : nullptr;
}

const ControllerImplementationMetadata* ControllerImplementationRegistry::find(
    ControllerImplementation implementation) {
    const ControllerImplementationMetadata* registered = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (registered[index].implementation == implementation) {
            return &registered[index];
        }
    }
    return nullptr;
}

const ControllerImplementationMetadata* ControllerImplementationRegistry::findByStableId(
    const char* stableId) {
    if (stableId == nullptr) return nullptr;
    const ControllerImplementationMetadata* registered = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (strcmp(registered[index].stableId, stableId) == 0) {
            return &registered[index];
        }
    }
    return nullptr;
}

} // namespace EnvNode
