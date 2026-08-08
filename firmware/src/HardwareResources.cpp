#include "HardwareResources.h"

namespace WeatherStation {
namespace {

const BoardGpioCapability CurrentBoardGpios[] = {
    {GpioResource(27), true, false},
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

HardwareResourceValidationResult BoardCapabilities::validate(
    HardwareInterfaceKind interfaceKind,
    const HardwareResourceAssignment& assignment) const {
    if (interfaceKind == HardwareInterfaceKind::Simulation) {
        return assignment.kind == HardwareResourceKind::None
            ? HardwareResourceValidationResult::Valid
            : HardwareResourceValidationResult::ResourceNotRequired;
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

} // namespace WeatherStation
