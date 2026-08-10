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
    I2C,
};

struct GpioResource {
    explicit GpioResource(uint8_t gpioNumber = 0)
        : number(gpioNumber) {
    }

    uint8_t number;
};

enum class I2CBus : uint8_t {
    I2C0 = 0,
    I2C1 = 1,
};

struct I2CResource {
    I2CResource(I2CBus busNumber = I2CBus::I2C0, uint8_t deviceAddress = 0x76)
        : bus(busNumber)
        , address(deviceAddress) {
    }

    I2CBus bus;
    uint8_t address;
};

struct HardwareResourceAssignment {
    HardwareResourceKind kind = HardwareResourceKind::None;
    GpioResource gpio;
    I2CResource i2c;

    static HardwareResourceAssignment none();
    static HardwareResourceAssignment gpioResource(GpioResource resource);
    static HardwareResourceAssignment i2cResource(I2CResource resource);
};

struct BoardGpioCapability {
    GpioResource resource;
    bool available;
    bool reserved;
};

struct BoardI2CBusCapability {
    I2CBus bus;
    GpioResource sda;
    GpioResource scl;
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
    const BoardI2CBusCapability* i2cBus(I2CBus bus) const;
    size_t i2cBusCount() const;
    const BoardI2CBusCapability* i2cBusAt(size_t index) const;
    HardwareResourceValidationResult validate(
        HardwareInterfaceKind interfaceKind,
        const HardwareResourceAssignment& assignment) const;

private:
    BoardCapabilities(const BoardGpioCapability* gpios, size_t gpioCount,
        const BoardI2CBusCapability* i2cBuses, size_t i2cBusCount);

    const BoardGpioCapability* gpios_;
    size_t gpioCount_;
    const BoardI2CBusCapability* i2cBuses_;
    size_t i2cBusCount_;
};

const char* i2cBusName(I2CBus bus);

bool exclusiveHardwareResourceConflict(
    const HardwareResourceAssignment& first,
    const HardwareResourceAssignment& second);

const char* hardwareInterfaceKindName(HardwareInterfaceKind kind);

} // namespace WeatherStation
