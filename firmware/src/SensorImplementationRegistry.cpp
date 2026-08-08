#include "SensorImplementationRegistry.h"

namespace WeatherStation {
namespace {

constexpr size_t ImplementationCount = 4;

const SensorImplementationMetadata* implementations() {
    static const SensorImplementationMetadata registeredImplementations[ImplementationCount] = {
        {SensorImplementation::SimulatedTemperature, "simulated_temperature", "Simulated Temperature",
            SensorProvenance::Simulated, {MeasurementType::Temperature, MeasurementType::Unknown}, 1,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(5000),
            "No implementation-specific configuration"},
        {SensorImplementation::SimulatedHumidity, "simulated_humidity", "Simulated Humidity",
            SensorProvenance::Simulated, {MeasurementType::RelativeHumidity, MeasurementType::Unknown}, 1,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(5000),
            "No implementation-specific configuration"},
        {SensorImplementation::SimulatedPressure, "simulated_pressure", "Simulated Pressure",
            SensorProvenance::Simulated, {MeasurementType::AtmosphericPressure, MeasurementType::Temperature}, 2,
            HardwareInterfaceKind::Simulation, "Firmware simulation", SensorSchedule::periodic(10000),
            "No implementation-specific configuration"},
        {SensorImplementation::AM2302, "am2302", "AM2302 / DHT22",
            SensorProvenance::Physical, {MeasurementType::Temperature, MeasurementType::RelativeHumidity}, 2,
            HardwareInterfaceKind::GPIO, "Custom single-wire protocol", SensorSchedule::periodic(5000),
            "AM2302Configuration: GPIO resource"},
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

} // namespace WeatherStation
