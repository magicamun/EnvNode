#include "HardwareDescriptorCompatibility.h"

#include <cstdlib>

#include "FirmwareVersion.h"

namespace EnvNode {
namespace {

bool versionLessThan(DescriptorSemanticVersion left, DescriptorSemanticVersion right) {
    if (left.major != right.major) return left.major < right.major;
    if (left.minor != right.minor) return left.minor < right.minor;
    return left.patch < right.patch;
}

bool matches(
    const DescriptorIdentifier& identifier,
    uint16_t code,
    const char* text) {
    return identifier.coded
        ? identifier.code == code
        : identifier.text.equals(text);
}

bool supportedDriver(const DescriptorContract& driver) {
    return driver.apiVersion == 1
        && matches(driver.id,
            static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff),
            "org.envnode.driver.gpio-on-off");
}

bool supportedCapability(const DescriptorIdentifier& capability) {
    return matches(capability,
        static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff),
        "actuator.on-off");
}

const BoardGpioCapability* gpioCapability(
    const BoardProfile& board,
    GpioResource resource) {
    for (size_t index = 0; index < board.gpioCount; ++index) {
        if (board.gpios[index].resource.number == resource.number) return &board.gpios[index];
    }
    return nullptr;
}

bool hasI2CBus(const BoardProfile& board, I2CBus bus) {
    for (size_t index = 0; index < board.i2cBusCount; ++index) {
        if (board.i2cBuses[index].bus == bus) return true;
    }
    return false;
}

GpioCapability requiredGpioCapabilities(const DescriptorCapabilitySet& capabilities) {
    GpioCapability result = GpioCapability::None;
    if (capabilities.contains(HardwareDescriptorCapabilityCode::DigitalInput)) {
        result = result | GpioCapability::DigitalInput;
    }
    if (capabilities.contains(HardwareDescriptorCapabilityCode::DigitalOutput)) {
        result = result | GpioCapability::DigitalOutput;
    }
    if (capabilities.contains(HardwareDescriptorCapabilityCode::AnalogInput)) {
        result = result | GpioCapability::AnalogInput;
    }
    return result;
}

} // namespace

DescriptorSemanticVersion currentFirmwareDescriptorVersion() {
    DescriptorSemanticVersion version;
    const char* cursor = FirmwareVersion;
    char* end = nullptr;
    version.major = static_cast<uint16_t>(std::strtoul(cursor, &end, 10));
    if (end == cursor || *end != '.') return {};
    cursor = end + 1;
    version.minor = static_cast<uint16_t>(std::strtoul(cursor, &end, 10));
    if (end == cursor || *end != '.') return {};
    cursor = end + 1;
    version.patch = static_cast<uint16_t>(std::strtoul(cursor, &end, 10));
    return end == cursor ? DescriptorSemanticVersion() : version;
}

HardwareDescriptorCompatibilityResult evaluateModuleDescriptorCompatibility(
    const HardwareDescriptor& descriptor,
    DescriptorSemanticVersion firmwareVersion,
    const BoardProfile& board,
    ModuleSlot slotId) {
    HardwareDescriptorCompatibilityResult result;
    if (versionLessThan(firmwareVersion, descriptor.compatibility.minimumFirmwareVersion)) {
        result.status = HardwareDescriptorCompatibilityStatus::FirmwareTooOld;
        return result;
    }
    if (descriptor.compatibility.platform.apiVersion != 1
        || !matches(descriptor.compatibility.platform.id,
            static_cast<uint8_t>(HardwareDescriptorPlatformCode::Any),
            "org.envnode.platform.any")) {
        result.status = HardwareDescriptorCompatibilityStatus::UnsupportedPlatform;
        return result;
    }
    if (descriptor.compatibility.safetyProfile.apiVersion != 1
        || !matches(descriptor.compatibility.safetyProfile.id,
            static_cast<uint8_t>(HardwareDescriptorSafetyProfileCode::ModuleInterface),
            "org.envnode.safety.module-interface")) {
        result.status = HardwareDescriptorCompatibilityStatus::UnsupportedSafetyProfile;
        return result;
    }
    if (!matches(descriptor.interfaceId,
            static_cast<uint8_t>(HardwareDescriptorInterfaceCode::Module2x7),
            "org.envnode.interface.module-2x7")) {
        result.status = HardwareDescriptorCompatibilityStatus::UnsupportedInterface;
        return result;
    }
    for (size_t index = 0; index < descriptor.compatibility.driverCount; ++index) {
        if (!supportedDriver(descriptor.compatibility.drivers[index])) {
            result.status = HardwareDescriptorCompatibilityStatus::MissingDriver;
            result.failedIndex = index;
            return result;
        }
    }
    for (size_t index = 0; index < descriptor.compatibility.capabilityCount; ++index) {
        if (!supportedCapability(descriptor.compatibility.capabilities[index])) {
            result.status = HardwareDescriptorCompatibilityStatus::MissingCapability;
            result.failedIndex = index;
            return result;
        }
    }

    const BoardModuleSlotCapability* slot = boardModuleSlot(board, slotId);
    if (slot == nullptr) {
        result.status = HardwareDescriptorCompatibilityStatus::SlotUnavailable;
        return result;
    }
    for (size_t index = 0; index < descriptor.requirementCount; ++index) {
        result.failedIndex = index;
        const DescriptorRequirement& requirement = descriptor.requirements[index];
        if (requirement.capabilities.hasUnknown) {
            result.status = HardwareDescriptorCompatibilityStatus::ResourceCapabilityMismatch;
            return result;
        }
        if (requirement.resource.equals("I2C0") || requirement.resource.equals("I2C1")) {
            if (requirement.kind != HardwareDescriptorResourceKind::I2C) {
                result.status = HardwareDescriptorCompatibilityStatus::ResourceKindMismatch;
                return result;
            }
            const I2CBus bus = requirement.resource.equals("I2C0")
                ? I2CBus::I2C0 : I2CBus::I2C1;
            if (!hasI2CBus(board, bus)) {
                result.status = HardwareDescriptorCompatibilityStatus::ResourceUnavailable;
                return result;
            }
            continue;
        }
        if (requirement.resource.equals("+5V")) {
            if (requirement.kind != HardwareDescriptorResourceKind::Power) {
                result.status = HardwareDescriptorCompatibilityStatus::ResourceKindMismatch;
                return result;
            }
            if (!slot->fiveVoltSupplyAvailable) {
                result.status = HardwareDescriptorCompatibilityStatus::ResourceUnavailable;
                return result;
            }
            continue;
        }

        GpioResource gpio;
        if (requirement.resource.equals("AUX_GPIO1")) gpio = slot->auxGpio1;
        else if (requirement.resource.equals("AUX_GPIO2")) gpio = slot->auxGpio2;
        else if (requirement.resource.equals("SPI_CS")) gpio = slot->spiChipSelect;
        else {
            result.status = HardwareDescriptorCompatibilityStatus::ResourceUnavailable;
            return result;
        }
        if (requirement.kind != HardwareDescriptorResourceKind::Gpio) {
            result.status = HardwareDescriptorCompatibilityStatus::ResourceKindMismatch;
            return result;
        }
        const BoardGpioCapability* available = gpioCapability(board, gpio);
        if (available == nullptr) {
            result.status = HardwareDescriptorCompatibilityStatus::ResourceUnavailable;
            return result;
        }
        const GpioCapability required = requiredGpioCapabilities(requirement.capabilities);
        if (!hasGpioCapabilities(available->capabilities, required)) {
            result.status = HardwareDescriptorCompatibilityStatus::ResourceCapabilityMismatch;
            return result;
        }
    }
    result.status = HardwareDescriptorCompatibilityStatus::Compatible;
    return result;
}

const char* hardwareDescriptorCompatibilityStatusName(
    HardwareDescriptorCompatibilityStatus status) {
    switch (status) {
        case HardwareDescriptorCompatibilityStatus::Compatible: return "Compatible";
        case HardwareDescriptorCompatibilityStatus::FirmwareTooOld: return "FirmwareTooOld";
        case HardwareDescriptorCompatibilityStatus::UnsupportedPlatform: return "UnsupportedPlatform";
        case HardwareDescriptorCompatibilityStatus::UnsupportedSafetyProfile: return "UnsupportedSafetyProfile";
        case HardwareDescriptorCompatibilityStatus::UnsupportedInterface: return "UnsupportedInterface";
        case HardwareDescriptorCompatibilityStatus::MissingDriver: return "MissingDriver";
        case HardwareDescriptorCompatibilityStatus::MissingCapability: return "MissingCapability";
        case HardwareDescriptorCompatibilityStatus::SlotUnavailable: return "SlotUnavailable";
        case HardwareDescriptorCompatibilityStatus::ResourceUnavailable: return "ResourceUnavailable";
        case HardwareDescriptorCompatibilityStatus::ResourceKindMismatch: return "ResourceKindMismatch";
        case HardwareDescriptorCompatibilityStatus::ResourceCapabilityMismatch: return "ResourceCapabilityMismatch";
        default: return "UnsupportedPlatform";
    }
}

} // namespace EnvNode
