#include "ModuleActuatorComposer.h"

#include <cstring>
#include <new>

namespace EnvNode {
namespace {

const ModuleDeviceBindingResolution* outputBinding(
    const ModuleDeviceInventoryEntry& entry) {
    for (size_t index = 0; index < entry.bindingCount; ++index) {
        if (entry.bindings[index].name.equals("output")
            && entry.bindings[index].resolved
            && entry.bindings[index].kind == HardwareDescriptorResourceKind::Gpio) {
            return &entry.bindings[index];
        }
    }
    return nullptr;
}

bool conflictsWithSensor(
    const HardwareResourceAssignment& resource,
    const SensorSlotConfiguration* sensors) {
    if (sensors == nullptr) return false;
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        if (sensors[index].enabled
            && sensors[index].implementation != SensorImplementation::None
            && exclusiveHardwareResourceConflict(resource, sensors[index].hardware)) {
            return true;
        }
    }
    return false;
}

void copyText(char* target, const DescriptorTextView& source) {
    const size_t length = source.size < MaxActuatorSlotNameLength
        ? source.size : MaxActuatorSlotNameLength;
    if (length != 0 && source.data != nullptr) memcpy(target, source.data, length);
    target[length] = '\0';
}

} // namespace

ModuleActuatorComposer::~ModuleActuatorComposer() {
    delete[] definitions_;
}

size_t ModuleActuatorComposer::compose(
    const ModuleDiscoveryService& discovery,
    const BoardProfile& board,
    const SensorSlotConfiguration* sensors) {
    count_ = 0;
    if (definitions_ == nullptr) {
        definitions_ = new (std::nothrow)
            AutomaticActuatorDefinition[MaxActuatorSlotCount];
    }
    if (definitions_ == nullptr) return 0;
    ModuleDeviceInventoryEntry* inventory =
        new (std::nothrow) ModuleDeviceInventoryEntry[MaximumDescriptorDevices];
    if (inventory == nullptr) return 0;
    for (size_t slotIndex = 0;
         slotIndex < ModuleDiscoveryService::SlotCount;
         ++slotIndex) {
        const ModuleSlot slot = static_cast<ModuleSlot>(slotIndex);
        const ModuleDiscoveryResult* module = discovery.result(slot);
        if (module == nullptr
            || module->source != ModuleDiscoverySource::Descriptor
            || module->descriptorCompatibility
                != HardwareDescriptorCompatibilityStatus::Compatible) {
            continue;
        }
        size_t inventoryCount = 0;
        if (!deriveModuleDeviceInventory(
                module->descriptor, board, slot, inventory,
                MaximumDescriptorDevices, inventoryCount)) {
            continue;
        }
        for (size_t index = 0;
             index < inventoryCount && count_ < MaxActuatorSlotCount;
             ++index) {
            const ModuleDeviceInventoryEntry& entry = inventory[index];
            if (entry.status != ModuleDeviceInventoryStatus::Ready) continue;
            const ModuleDeviceBindingResolution* output = outputBinding(entry);
            if (output == nullptr) continue;
            const HardwareResourceAssignment hardware =
                HardwareResourceAssignment::gpioResource(output->gpio);
            if (conflictsWithSensor(hardware, sensors)) continue;

            AutomaticActuatorDefinition& definition = definitions_[count_++];
            definition = AutomaticActuatorDefinition{};
            definition.moduleSlot = slot;
            definition.hasModuleInstanceId = module->descriptorHasInstanceId;
            if (definition.hasModuleInstanceId) {
                memcpy(definition.moduleInstanceFingerprint,
                    module->descriptorInstanceId,
                    sizeof(definition.moduleInstanceFingerprint));
            }
            copyText(definition.deviceId, entry.id);
            snprintf(definition.name, sizeof(definition.name), "%s %s",
                moduleSlotName(slot), definition.deviceId);
            definition.implementation = ActuatorImplementation::GpioOnOff;
            definition.hardware = hardware;
        }
    }
    delete[] inventory;
    return count_;
}

const AutomaticActuatorDefinition* ModuleActuatorComposer::definitions() const {
    return definitions_;
}

size_t ModuleActuatorComposer::count() const {
    return count_;
}

} // namespace EnvNode
