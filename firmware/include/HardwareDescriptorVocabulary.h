#pragma once

#include <cstdint>

namespace EnvNode {

enum class HardwareDescriptorKey : uint8_t {
    SchemaVersion = 0,
    ObjectKind = 1,
    Identity = 2,
    Compatibility = 3,
    Description = 4,
    Hardware = 5,
    Manufacturing = 6,
    Calibration = 7,
    TypeId = 8,
    InstanceId = 9,
    Manufacturer = 10,
    HardwareRevision = 11,
    LegacyProfileId = 12,
    Major = 13,
    Minor = 14,
    MinimumFirmwareVersion = 15,
    Platform = 16,
    SafetyProfile = 17,
    Drivers = 18,
    Capabilities = 19,
    Id = 20,
    ApiVersion = 21,
    Name = 22,
    Summary = 23,
    DocumentationUrl = 24,
    Resources = 25,
    Slots = 26,
    Kind = 27,
    PlatformBinding = 28,
    VoltageMillivolts = 29,
    Properties = 30,
    Interface = 31,
    IdentityAddress = 32,
    Bindings = 33,
    Requirements = 34,
    Devices = 35,
    Resource = 36,
    Driver = 37,
    Parameters = 38,
    Measurements = 39,
    SerialNumber = 40,
    ProductionBatch = 41,
    ProductionDate = 42,
    Target = 43,
    Schema = 44,
    Values = 45,
};

enum class HardwareDescriptorResourceKind : uint8_t {
    Gpio = 0,
    I2C = 1,
    SPI = 2,
    Power = 3,
};

enum class HardwareDescriptorDeviceKind : uint8_t {
    Sensor = 0,
    Actuator = 1,
    Infrastructure = 2,
};

enum class HardwareDescriptorPlatformCode : uint8_t {
    Esp32 = 1,
    Any = 2,
};

enum class HardwareDescriptorSafetyProfileCode : uint8_t {
    Esp32EnvNodeMini = 1,
    ModuleInterface = 2,
};

enum class HardwareDescriptorInterfaceCode : uint8_t {
    Module2x7 = 1,
};

enum class HardwareDescriptorDriverCode : uint8_t {
    GpioOnOff = 1,
    Ads1115 = 2,
};

enum class HardwareDescriptorCapabilityCode : uint8_t {
    DigitalInput = 1,
    DigitalOutput = 2,
    AnalogInput = 3,
    Supply = 4,
    ActuatorOnOff = 5,
    SensorPressure = 6,
    SensorCurrent = 7,
};

} // namespace EnvNode
