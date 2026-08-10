#include "HardwareResources.h"

namespace WeatherStation {
namespace {

const BoardGpioCapability CurrentBoardGpios[] = {
    {GpioResource(25), true, false},
    {GpioResource(26), true, false},
    {GpioResource(27), true, false},
    {GpioResource(32), true, false},
    {GpioResource(33), true, false},
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

BoardCapabilities::BoardCapabilities(const BoardGpioCapability* gpios, size_t gpioCount)
    : gpios_(gpios)
    , gpioCount_(gpioCount) {
}

const BoardCapabilities& BoardCapabilities::current() {
    static const BoardCapabilities capabilities(
        CurrentBoardGpios,
        sizeof(CurrentBoardGpios) / sizeof(CurrentBoardGpios[0]));
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

HardwareResourceValidationResult BoardCapabilities::validate(
    HardwareInterfaceKind interfaceKind,
    const HardwareResourceAssignment& assignment) const {
    if (interfaceKind == HardwareInterfaceKind::Simulation) {
        return assignment.kind == HardwareResourceKind::None
            ? HardwareResourceValidationResult::Valid
            : HardwareResourceValidationResult::ResourceNotRequired;
    }
    if (interfaceKind == HardwareInterfaceKind::I2C) {
        if (assignment.kind != HardwareResourceKind::I2C) {
            return HardwareResourceValidationResult::ResourceKindMismatch;
        }
        return assignment.i2c.bus == I2CBus::I2C0
                && assignment.i2c.address >= 0x08
                && assignment.i2c.address <= 0x77
            ? HardwareResourceValidationResult::Valid
            : HardwareResourceValidationResult::ResourceDoesNotExist;
    }
    if (interfaceKind != HardwareInterfaceKind::GPIO) {
        return HardwareResourceValidationResult::ResourceKindMismatch;
    }
    if (assignment.kind != HardwareResourceKind::GPIO) {
        return HardwareResourceValidationResult::ResourceKindMismatch;
    }
    const BoardGpioCapability* capability = gpio(assignment.gpio);
    if (capability == nullptr) return HardwareResourceValidationResult::ResourceDoesNotExist;
    if (capability->reserved) return HardwareResourceValidationResult::ResourceReserved;
    if (!capability->available) return HardwareResourceValidationResult::ResourceUnavailable;
    return HardwareResourceValidationResult::Valid;
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

} // namespace WeatherStation
