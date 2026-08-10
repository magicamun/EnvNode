#include "HardwareResources.h"

namespace EnvNode {
namespace {

const BoardGpioCapability CurrentBoardGpios[] = {
    {GpioResource(4), "GPIO4", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(13), "GPIO13", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(14), "GPIO14", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(15), "GPIO15", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(16), "GPIO16", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(17), "GPIO17", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(18), "GPIO18", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(19), "GPIO19", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(23), "GPIO23", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup},
    {GpioResource(32), "GPIO32", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(33), "GPIO33", GpioCapability::DigitalInput | GpioCapability::DigitalOutput | GpioCapability::Interrupt | GpioCapability::InternalPullup | GpioCapability::AnalogInput},
    {GpioResource(34), "GPIO34", GpioCapability::AnalogInput},
    {GpioResource(35), "GPIO35", GpioCapability::AnalogInput},
    {GpioResource(36), "GPIO36", GpioCapability::AnalogInput},
    {GpioResource(39), "GPIO39", GpioCapability::AnalogInput},
};

const BoardI2CBusCapability CurrentBoardI2CBuses[] = {
    {I2CBus::I2C0, GpioResource(21), GpioResource(22)},
    {I2CBus::I2C1, GpioResource(25), GpioResource(26)},
};

} // namespace

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
    static const BoardCapabilities capabilities(
        CurrentBoardGpios,
        sizeof(CurrentBoardGpios) / sizeof(CurrentBoardGpios[0]),
        CurrentBoardI2CBuses,
        sizeof(CurrentBoardI2CBuses) / sizeof(CurrentBoardI2CBuses[0]));
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

} // namespace EnvNode
