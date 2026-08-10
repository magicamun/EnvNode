#include "SensorImplementationRegistry.h"

#include <cstring>

namespace WeatherStation {
namespace {

constexpr size_t ImplementationCount = 8;

const SensorImplementationMetadata* implementations() {
    static const SensorImplementationMetadata registeredImplementations[ImplementationCount] = {
        {SensorImplementation::None, "none", "None",
            SensorProvenance::Simulated, {MeasurementType::Unknown, MeasurementType::Unknown, MeasurementType::Unknown}, 0,
            HardwareInterfaceKind::Simulation, "No runtime sensor", SensorSchedule::eventOnly(false),
            "No implementation-specific configuration"},
        {SensorImplementation::SimulatedTemperature, "simulated_temperature", "Simulated Temperature",
            SensorProvenance::Simulated, {MeasurementType::Temperature, MeasurementType::Unknown, MeasurementType::Unknown}, 1,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(5000),
            "No implementation-specific configuration"},
        {SensorImplementation::SimulatedHumidity, "simulated_humidity", "Simulated Humidity",
            SensorProvenance::Simulated, {MeasurementType::RelativeHumidity, MeasurementType::Unknown, MeasurementType::Unknown}, 1,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(5000),
            "No implementation-specific configuration"},
        {SensorImplementation::SimulatedPressure, "simulated_pressure", "Simulated Pressure",
            SensorProvenance::Simulated, {MeasurementType::AtmosphericPressure, MeasurementType::Temperature, MeasurementType::Unknown}, 2,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(10000),
            "No implementation-specific configuration"},
        {SensorImplementation::AM2302, "am2302", "AM2302 / DHT22",
            SensorProvenance::Physical, {MeasurementType::Temperature, MeasurementType::RelativeHumidity, MeasurementType::Unknown}, 2,
            HardwareInterfaceKind::GPIO, "Custom single-wire protocol", SensorSchedule::periodic(5000),
            "AM2302Configuration: GPIO resource"},
        {SensorImplementation::RainGauge, "rain_gauge", "Rain Gauge",
            SensorProvenance::Physical, {MeasurementType::RainGaugeTip, MeasurementType::RainfallIncrement, MeasurementType::Unknown}, 2,
            HardwareInterfaceKind::GPIO, "Digital interrupt", SensorSchedule::eventOnly(true),
            "RainGaugeConfiguration: GPIO, millimetres per tip, debounce"},
        {SensorImplementation::BME280, "bme280", "BME280",
            SensorProvenance::Physical, {MeasurementType::Temperature, MeasurementType::RelativeHumidity, MeasurementType::AtmosphericPressure}, 3,
            HardwareInterfaceKind::I2C, "I2C", SensorSchedule::periodic(5000),
            "BME280Configuration: I2C bus and address"},
        {SensorImplementation::SHT4x, "sht4x", "SHT4x",
            SensorProvenance::Physical, {MeasurementType::Temperature, MeasurementType::RelativeHumidity, MeasurementType::Unknown}, 2,
            HardwareInterfaceKind::I2C, "I2C", SensorSchedule::periodic(5000),
            "SHT4xConfiguration: I2C bus and address"},
    };
    return registeredImplementations;
}

} // namespace

size_t SensorImplementationRegistry::count() {
    return ImplementationCount;
}

const SensorImplementationMetadata* SensorImplementationRegistry::at(size_t index) {
    return index < count() ? &implementations()[index] : nullptr;
}

const SensorImplementationMetadata* SensorImplementationRegistry::find(
    SensorImplementation implementation) {
    const SensorImplementationMetadata* registeredImplementations = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (registeredImplementations[index].implementation == implementation) {
            return &registeredImplementations[index];
        }
    }
    return nullptr;
}

const SensorImplementationMetadata* SensorImplementationRegistry::findByStableId(
    const char* stableId) {
    if (stableId == nullptr) return nullptr;
    const SensorImplementationMetadata* registeredImplementations = implementations();
    for (size_t index = 0; index < count(); ++index) {
        if (strcmp(registeredImplementations[index].stableId, stableId) == 0) {
            return &registeredImplementations[index];
        }
    }
    return nullptr;
}

} // namespace WeatherStation
