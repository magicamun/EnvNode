#include "HardwareResources.h"
#include "BoardProfile.h"

namespace EnvNode {

HardwareResourceAssignment HardwareResourceAssignment::none() {
    return HardwareResourceAssignment{};
}

HardwareResourceAssignment HardwareResourceAssignment::gpioResource(GpioResource resource) {
    HardwareResourceAssignment assignment;
    assignment.kind = HardwareResourceKind::GPIO;
    assignment.gpio = resource;
    return assignment;
}

HardwareResourceAssignment HardwareResourceAssignment::i2cResource(I2CResource resource) {
    HardwareResourceAssignment assignment;
    assignment.kind = HardwareResourceKind::I2C;
    assignment.i2c = resource;
    return assignment;
}

BoardCapabilities::BoardCapabilities(const BoardGpioCapability* gpios, size_t gpioCount,
    const BoardI2CBusCapability* i2cBuses, size_t i2cBusCount)
    : gpios_(gpios)
    , gpioCount_(gpioCount)
    , i2cBuses_(i2cBuses)
    , i2cBusCount_(i2cBusCount) {
}

const BoardCapabilities& BoardCapabilities::current() {
    const BoardProfile& profile = currentBoardProfile();
    static const BoardCapabilities capabilities(
        profile.gpios, profile.gpioCount, profile.i2cBuses, profile.i2cBusCount);
    return capabilities;
}

const BoardGpioCapability* BoardCapabilities::gpio(GpioResource resource) const {
    for (size_t index = 0; index < gpioCount_; ++index) {
        if (gpios_[index].resource.number == resource.number) {
            return &gpios_[index];
        }
    }
    return nullptr;
}

size_t BoardCapabilities::gpioCount() const {
    return gpioCount_;
}

const BoardGpioCapability* BoardCapabilities::gpioAt(size_t index) const {
    return index < gpioCount_ ? &gpios_[index] : nullptr;
}

const BoardI2CBusCapability* BoardCapabilities::i2cBus(I2CBus bus) const {
    for (size_t index = 0; index < i2cBusCount_; ++index) {
        if (i2cBuses_[index].bus == bus) return &i2cBuses_[index];
    }
    return nullptr;
}

size_t BoardCapabilities::i2cBusCount() const { return i2cBusCount_; }

const BoardI2CBusCapability* BoardCapabilities::i2cBusAt(size_t index) const {
    return index < i2cBusCount_ ? &i2cBuses_[index] : nullptr;
}

HardwareResourceValidationResult BoardCapabilities::validate(
    HardwareInterfaceKind interfaceKind,
    const HardwareResourceAssignment& assignment,
    GpioCapability requiredGpioCapabilities) const {
    if (interfaceKind == HardwareInterfaceKind::Simulation) {
        return assignment.kind == HardwareResourceKind::None
            ? HardwareResourceValidationResult::Valid
            : HardwareResourceValidationResult::ResourceNotRequired;
    }
    if (interfaceKind == HardwareInterfaceKind::I2C) {
        if (assignment.kind != HardwareResourceKind::I2C) {
            return HardwareResourceValidationResult::ResourceKindMismatch;
        }
        return i2cBus(assignment.i2c.bus) != nullptr
                && assignment.i2c.address >= 0x08
                && assignment.i2c.address <= 0x77
            ? HardwareResourceValidationResult::Valid
            : HardwareResourceValidationResult::ResourceDoesNotExist;
    }
    if (interfaceKind != HardwareInterfaceKind::GPIO
        && interfaceKind != HardwareInterfaceKind::ADC) {
        return HardwareResourceValidationResult::ResourceKindMismatch;
    }
    if (assignment.kind != HardwareResourceKind::GPIO) {
        return HardwareResourceValidationResult::ResourceKindMismatch;
    }
    const BoardGpioCapability* capability = gpio(assignment.gpio);
    if (capability == nullptr) return HardwareResourceValidationResult::ResourceDoesNotExist;
    const GpioCapability required = interfaceKind == HardwareInterfaceKind::ADC
        ? requiredGpioCapabilities | GpioCapability::AnalogInput
        : requiredGpioCapabilities;
    if (!hasGpioCapabilities(capability->capabilities, required)) {
        return HardwareResourceValidationResult::ResourceUnavailable;
    }
    return HardwareResourceValidationResult::Valid;
}

const char* i2cBusName(I2CBus bus) {
    switch (bus) {
        case I2CBus::I2C0: return "I2C0";
        case I2CBus::I2C1: return "I2C1";
        default: return "unknown I2C bus";
    }
}

const char* hardwareInterfaceKindName(HardwareInterfaceKind kind) {
    switch (kind) {
        case HardwareInterfaceKind::Simulation: return "Simulation";
        case HardwareInterfaceKind::GPIO: return "GPIO";
        case HardwareInterfaceKind::I2C: return "I2C";
        case HardwareInterfaceKind::OneWire: return "OneWire";
        case HardwareInterfaceKind::ADC: return "ADC";
        case HardwareInterfaceKind::SPI: return "SPI";
        case HardwareInterfaceKind::Custom: return "Custom";
        default: return "Custom";
    }
}

bool exclusiveHardwareResourceConflict(
    const HardwareResourceAssignment& first,
    const HardwareResourceAssignment& second) {
    const bool gpioConflict = first.kind == HardwareResourceKind::GPIO
        && second.kind == HardwareResourceKind::GPIO
        && first.gpio.number == second.gpio.number;
    const bool i2cConflict = first.kind == HardwareResourceKind::I2C
        && second.kind == HardwareResourceKind::I2C
        && first.i2c.bus == second.i2c.bus
        && first.i2c.address == second.i2c.address;
    return gpioConflict || i2cConflict;
}

bool validateExclusiveHardwareResourceOccupancy(
    const HardwareResourceClaim* claims,
    size_t claimCount) {
    if (claims == nullptr && claimCount != 0) return false;
    for (size_t index = 0; index < claimCount; ++index) {
        if (!claims[index].active) continue;
        for (size_t other = index + 1; other < claimCount; ++other) {
            if (claims[other].active
                && exclusiveHardwareResourceConflict(
                    claims[index].assignment, claims[other].assignment)) {
                return false;
            }
        }
    }
    return true;
}

} // namespace EnvNode
