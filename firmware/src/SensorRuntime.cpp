#include "SensorRuntime.h"

#include "SensorImplementationRegistry.h"

namespace WeatherStation {

SensorRuntime::SensorRuntime(
    IConfigurationService& configurationService,
    SensorFactory& firstFactory,
    SensorFactory& secondFactory,
    SensorManager& sensorManager)
    : configurationService_(configurationService)
    , activeFactory_(&firstFactory)
    , inactiveFactory_(&secondFactory)
    , sensorManager_(sensorManager) {
}

bool SensorRuntime::construct(
    SensorFactory& factory,
    const SensorSlotConfiguration* slots,
    ISensor** sensors,
    size_t& activeSensorCount,
    const char*& failureReason) {
    activeSensorCount = 0;
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        sensors[index] = nullptr;
        SensorFactoryResult result;
        sensors[index] = factory.create(index, slots[index], result);
        if (result == SensorFactoryResult::NoRuntimeSensor) continue;
        if (result != SensorFactoryResult::Created || sensors[index] == nullptr) {
            failureReason = "Sensor construction failed";
            factory.destroyAll();
            return false;
        }
        ++activeSensorCount;
    }
    return true;
}

bool SensorRuntime::registerComposition(
    const SensorSlotConfiguration* slots,
    ISensor* const* sensors,
    size_t& activeSensorCount,
    const char*& failureReason) {
    activeSensorCount = 0;
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        if (sensors[index] == nullptr) continue;
        const SensorSlotConfiguration& slot = slots[index];
        const SensorImplementationMetadata* implementation =
            SensorImplementationRegistry::find(slot.implementation);
        if (implementation == nullptr) {
            failureReason = "Registered implementation metadata unavailable";
            return false;
        }
        const SensorRegistrationResult registration = sensorManager_.registerSensor(
            *sensors[index],
            slot.schedule,
            SensorRegistrationMetadata(
                slot.name.c_str(),
                slot.implementation,
                hardwareInterfaceKindName(implementation->interfaceKind),
                implementation->protocolDescription,
                implementation->configurationSchemaDescription,
                slot.hardware));
        if (registration != SensorRegistrationResult::Registered) {
            failureReason = "Sensor registration failed";
            return false;
        }
        ++activeSensorCount;
    }
    return true;
}

void SensorRuntime::snapshotActiveConfiguration(
    const SensorSlotConfiguration* slots,
    ISensor* const* sensors) {
    for (size_t index = 0; index < MaxSensorSlotCount; ++index) {
        activeSlots_[index] = slots[index];
        activeSensors_[index] = sensors[index];
    }
}

bool SensorRuntime::initialize(size_t& activeSensorCount, const char*& failureReason) {
    const SensorSlotConfiguration* desired =
        configurationService_.getConfiguration().sensorSlots;
    ISensor* sensors[MaxSensorSlotCount] = {};
    if (!construct(*activeFactory_, desired, sensors, activeSensorCount, failureReason)) return false;
    if (!registerComposition(desired, sensors, activeSensorCount, failureReason)) {
        sensorManager_.clear();
        activeFactory_->destroyAll();
        return false;
    }
    snapshotActiveConfiguration(desired, sensors);
    return true;
}

bool SensorRuntime::rebuild(size_t& activeSensorCount, const char*& failureReason) {
    const SensorSlotConfiguration* desired =
        configurationService_.getConfiguration().sensorSlots;
    ISensor* stagedSensors[MaxSensorSlotCount] = {};
    if (!construct(*inactiveFactory_, desired, stagedSensors, activeSensorCount, failureReason)) {
        return false;
    }

    sensorManager_.clear();
    if (!registerComposition(desired, stagedSensors, activeSensorCount, failureReason)) {
        sensorManager_.clear();
        size_t restoredCount = 0;
        const char* restoreFailure = nullptr;
        const bool restored = registerComposition(
            activeSlots_, activeSensors_, restoredCount, restoreFailure);
        if (restored) sensorManager_.begin();
        inactiveFactory_->destroyAll();
        if (!restored) failureReason = "Rebuild and previous runtime restoration failed";
        return false;
    }

    activeFactory_->destroyAll();
    sensorManager_.begin();
    SensorFactory* previousFactory = activeFactory_;
    activeFactory_ = inactiveFactory_;
    inactiveFactory_ = previousFactory;
    snapshotActiveConfiguration(desired, stagedSensors);
    return true;
}

} // namespace WeatherStation
