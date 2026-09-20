#include "ModuleDeviceInventory.h"

namespace EnvNode {
namespace {

bool identifierMatches(
    const DescriptorIdentifier& identifier,
    uint16_t code,
    const char* text) {
    return identifier.coded ? identifier.code == code : identifier.text.equals(text);
}

const DescriptorRequirement* findRequirement(
    const HardwareDescriptor& descriptor,
    const DescriptorTextView& id) {
    for (size_t index = 0; index < descriptor.requirementCount; ++index) {
        if (descriptor.requirements[index].id.size == id.size) {
            bool equal = true;
            for (size_t byte = 0; byte < id.size; ++byte) {
                if (descriptor.requirements[index].id.data[byte] != id.data[byte]) {
                    equal = false;
                    break;
                }
            }
            if (equal) return &descriptor.requirements[index];
        }
    }
    return nullptr;
}

bool resolveRequirement(
    const DescriptorRequirement& requirement,
    const BoardModuleSlotCapability& slot,
    ModuleDeviceBindingResolution& resolution) {
    resolution.logicalResource = requirement.resource;
    resolution.kind = requirement.kind;
    if (requirement.kind == HardwareDescriptorResourceKind::Gpio) {
        if (requirement.resource.equals("AUX_GPIO1")) resolution.gpio = slot.auxGpio1;
        else if (requirement.resource.equals("AUX_GPIO2")) resolution.gpio = slot.auxGpio2;
        else if (requirement.resource.equals("SPI_CS")) resolution.gpio = slot.spiChipSelect;
        else return false;
        resolution.resolved = true;
        return true;
    }
    if (requirement.kind == HardwareDescriptorResourceKind::I2C) {
        if (requirement.resource.equals("I2C0")) resolution.i2cBus = I2CBus::I2C0;
        else if (requirement.resource.equals("I2C1")) resolution.i2cBus = I2CBus::I2C1;
        else return false;
        resolution.resolved = true;
        return true;
    }
    if (requirement.kind == HardwareDescriptorResourceKind::Power
        && requirement.resource.equals("+5V")
        && slot.fiveVoltSupplyAvailable) {
        resolution.resolved = true;
        return true;
    }
    return false;
}

} // namespace

bool deriveModuleDeviceInventory(
    const HardwareDescriptor& descriptor,
    const BoardProfile& board,
    ModuleSlot slotId,
    ModuleDeviceInventoryEntry* entries,
    size_t capacity,
    size_t& count) {
    count = 0;
    if (entries == nullptr || descriptor.deviceCount > capacity) return false;
    const BoardModuleSlotCapability* slot = boardModuleSlot(board, slotId);
    if (slot == nullptr) return false;

    count = descriptor.deviceCount;
    for (size_t index = 0; index < descriptor.deviceCount; ++index) {
        const DescriptorDevice& device = descriptor.devices[index];
        ModuleDeviceInventoryEntry& entry = entries[index];
        entry = {};
        entry.slot = slotId;
        entry.id = device.id;
        entry.kind = device.kind;
        entry.driver = device.driver;
        entry.capabilities = device.capabilities;
        entry.bindingCount = device.bindingCount;
        entry.hasActiveLevel = device.hasActiveLevel;
        entry.activeLevelHigh = device.activeLevelHigh;
        entry.hasSafeLevel = device.hasSafeLevel;
        entry.safeLevelHigh = device.safeLevelHigh;

        bool resourcesResolved = true;
        bool outputBindingResolved = false;
        for (size_t bindingIndex = 0; bindingIndex < device.bindingCount; ++bindingIndex) {
            ModuleDeviceBindingResolution& resolution = entry.bindings[bindingIndex];
            resolution.name = device.bindings[bindingIndex].name;
            resolution.target = device.bindings[bindingIndex].target;
            const DescriptorRequirement* requirement = findRequirement(
                descriptor, device.bindings[bindingIndex].target);
            if (requirement == nullptr) {
                resourcesResolved = false;
                entry.status = ModuleDeviceInventoryStatus::UnknownBindingTarget;
                continue;
            }
            if (!resolveRequirement(*requirement, *slot, resolution)) {
                resourcesResolved = false;
                entry.status = ModuleDeviceInventoryStatus::ResourceUnavailable;
            } else if (resolution.name.equals("output")
                && resolution.kind == HardwareDescriptorResourceKind::Gpio) {
                outputBindingResolved = true;
            }
        }

        if (device.kind != HardwareDescriptorDeviceKind::Actuator) {
            entry.status = ModuleDeviceInventoryStatus::UnsupportedDeviceKind;
        } else if (!identifierMatches(
                device.driver.id,
                static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff),
                "org.envnode.driver.gpio-on-off")
            || device.driver.apiVersion != 1) {
            entry.status = ModuleDeviceInventoryStatus::UnsupportedDriver;
        } else if (!device.capabilities.contains(
                HardwareDescriptorCapabilityCode::ActuatorOnOff)) {
            entry.status = ModuleDeviceInventoryStatus::MissingCapability;
        } else if (!outputBindingResolved) {
            entry.status = ModuleDeviceInventoryStatus::MissingBinding;
        } else if (!resourcesResolved) {
            // Keep the more specific resolution status assigned above.
        } else if (!device.hasActiveLevel || !device.hasSafeLevel
            || device.activeLevelHigh == device.safeLevelHigh) {
            entry.status = ModuleDeviceInventoryStatus::InvalidParameters;
        } else {
            entry.status = ModuleDeviceInventoryStatus::Ready;
        }
    }
    return true;
}

const char* moduleDeviceKindName(HardwareDescriptorDeviceKind kind) {
    switch (kind) {
        case HardwareDescriptorDeviceKind::Sensor: return "Sensor";
        case HardwareDescriptorDeviceKind::Actuator: return "Actuator";
        case HardwareDescriptorDeviceKind::Infrastructure: return "Infrastructure";
        default: return "Unknown";
    }
}

const char* moduleDeviceDriverName(const DescriptorContract& driver) {
    if (identifierMatches(driver.id,
            static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff),
            "org.envnode.driver.gpio-on-off")) {
        return "gpio-on-off";
    }
    if (identifierMatches(driver.id,
            static_cast<uint8_t>(HardwareDescriptorDriverCode::Ads1115),
            "org.envnode.driver.ads1115")) {
        return "ads1115";
    }
    return "unknown";
}

const char* moduleDeviceInventoryStatusName(ModuleDeviceInventoryStatus status) {
    switch (status) {
        case ModuleDeviceInventoryStatus::Ready: return "Ready";
        case ModuleDeviceInventoryStatus::UnsupportedDeviceKind: return "UnsupportedDeviceKind";
        case ModuleDeviceInventoryStatus::UnsupportedDriver: return "UnsupportedDriver";
        case ModuleDeviceInventoryStatus::MissingCapability: return "MissingCapability";
        case ModuleDeviceInventoryStatus::MissingBinding: return "MissingBinding";
        case ModuleDeviceInventoryStatus::UnknownBindingTarget: return "UnknownBindingTarget";
        case ModuleDeviceInventoryStatus::ResourceUnavailable: return "ResourceUnavailable";
        case ModuleDeviceInventoryStatus::InvalidParameters: return "InvalidParameters";
        default: return "UnsupportedDriver";
    }
}

} // namespace EnvNode
