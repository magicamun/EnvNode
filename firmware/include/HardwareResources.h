#pragma once

#include <cstddef>
#include <cstdint>

namespace WeatherStation {

enum class HardwareInterfaceKind {
    Simulation,
    GPIO,
    I2C,
    OneWire,
    ADC,
    SPI,
    Custom,
};

enum class HardwareResourceKind {
    None,
    GPIO,
};

struct GpioResource {
    explicit GpioResource(uint8_t gpioNumber = 0)
        : number(gpioNumber) {
    }

    uint8_t number;
};

struct HardwareResourceAssignment {
    HardwareResourceKind kind = HardwareResourceKind::None;
    GpioResource gpio;

    static HardwareResourceAssignment none();
    static HardwareResourceAssignment gpioResource(GpioResource resource);
};

struct BoardGpioCapability {
    GpioResource resource;
    bool available;
    bool reserved;
};

enum class HardwareResourceValidationResult {
    Valid,
    ResourceNotRequired,
    ResourceKindMismatch,
    ResourceDoesNotExist,
    ResourceUnavailable,
    ResourceReserved,
};

class IHardwareResourceConflictValidator {
public:
    virtual ~IHardwareResourceConflictValidator() = default;

    virtual bool conflicts(
        const HardwareResourceAssignment& existing,
        const HardwareResourceAssignment& candidate) const = 0;
};

class BoardCapabilities {
public:
    static const BoardCapabilities& current();

    const BoardGpioCapability* gpio(GpioResource resource) const;
    size_t gpioCount() const;
    const BoardGpioCapability* gpioAt(size_t index) const;
    HardwareResourceValidationResult validate(
        HardwareInterfaceKind interfaceKind,
        const HardwareResourceAssignment& assignment) const;

private:
    BoardCapabilities(const BoardGpioCapability* gpios, size_t gpioCount);

    const BoardGpioCapability* gpios_;
    size_t gpioCount_;
};

bool exclusiveHardwareResourceConflict(
    const HardwareResourceAssignment& first,
    const HardwareResourceAssignment& second);

const char* hardwareInterfaceKindName(HardwareInterfaceKind kind);

} // namespace WeatherStation
