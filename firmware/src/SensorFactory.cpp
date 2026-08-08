#include "SensorFactory.h"

#include <new>

#include "SensorImplementationRegistry.h"

namespace WeatherStation {

SensorFactory::SensorFactory(IMonotonicClock& monotonicClock, ILogger& logger)
    : monotonicClock_(monotonicClock)
    , logger_(logger) {
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        constructed_[index] = SensorImplementation::None;
    }
}

SensorFactory::~SensorFactory() {
    destroyAll();
}

ISensor* SensorFactory::create(
    size_t storageIndex,
    const SensorSlotConfiguration& slot,
    SensorFactoryResult& result) {
    if (storageIndex >= MaxSensorSlotCount) {
        result = SensorFactoryResult::InvalidStorageIndex;
        return nullptr;
    }
    if (constructed_[storageIndex] != SensorImplementation::None) {
        result = SensorFactoryResult::StorageOccupied;
        return nullptr;
    }
    if (!slot.enabled || slot.implementation == SensorImplementation::None) {
        result = SensorFactoryResult::NoRuntimeSensor;
        return nullptr;
    }
    const SensorImplementationMetadata* metadata =
        SensorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) {
        result = SensorFactoryResult::UnknownImplementation;
        return nullptr;
    }
    if (BoardCapabilities::current().validate(metadata->interfaceKind, slot.hardware)
        != HardwareResourceValidationResult::Valid) {
        result = SensorFactoryResult::InvalidResource;
        return nullptr;
    }
    if (slot.implementation == SensorImplementation::AM2302
        && (slot.hardware.kind != HardwareResourceKind::GPIO
            || slot.implementationConfiguration.am2302.gpio.number != slot.hardware.gpio.number)) {
        result = SensorFactoryResult::InvalidResource;
        return nullptr;
    }

    void* target = &storage_[storageIndex];
    ISensor* sensor = nullptr;
    switch (slot.implementation) {
        case SensorImplementation::SimulatedTemperature:
            sensor = new (target) SimulatedTemperatureSensor(slot.slotId, monotonicClock_);
            break;
        case SensorImplementation::SimulatedHumidity:
            sensor = new (target) SimulatedHumiditySensor(slot.slotId, monotonicClock_);
            break;
        case SensorImplementation::SimulatedPressure:
            sensor = new (target) SimulatedPressureSensor(slot.slotId, monotonicClock_);
            break;
        case SensorImplementation::AM2302:
            sensor = new (target) AM2302Sensor(
                slot.slotId,
                slot.implementationConfiguration.am2302.gpio.number,
                monotonicClock_,
                logger_);
            break;
        case SensorImplementation::None:
        default:
            result = SensorFactoryResult::UnknownImplementation;
            return nullptr;
    }
    constructed_[storageIndex] = slot.implementation;
    result = SensorFactoryResult::Created;
    return sensor;
}

void SensorFactory::destroy(size_t storageIndex) {
    void* target = &storage_[storageIndex];
    switch (constructed_[storageIndex]) {
        case SensorImplementation::SimulatedTemperature:
            static_cast<SimulatedTemperatureSensor*>(target)->~SimulatedTemperatureSensor();
            break;
        case SensorImplementation::SimulatedHumidity:
            static_cast<SimulatedHumiditySensor*>(target)->~SimulatedHumiditySensor();
            break;
        case SensorImplementation::SimulatedPressure:
            static_cast<SimulatedPressureSensor*>(target)->~SimulatedPressureSensor();
            break;
        case SensorImplementation::AM2302:
            static_cast<AM2302Sensor*>(target)->~AM2302Sensor();
            break;
        case SensorImplementation::None:
        default:
            return;
    }
    constructed_[storageIndex] = SensorImplementation::None;
}

void SensorFactory::destroyAll() {
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) destroy(index);
}

} // namespace WeatherStation
