#include "SensorRuntime.h"

#include "SensorImplementationRegistry.h"
#include <new>

namespace EnvNode {

SensorRuntime::SensorRuntime(
    IConfigurationService& configurationService,
    SensorFactory& firstFactory,
    SensorFactory& secondFactory,
    SensorManager& sensorManager,
    const IMeasurementResolver& measurements,
    const IMonotonicClock& clock)
    : configurationService_(configurationService)
    , activeFactory_(&firstFactory)
    , inactiveFactory_(&secondFactory)
    , sensorManager_(sensorManager)
    , measurements_(measurements)
    , clock_(clock)
    , activePressure_(new (std::nothrow) ActivePressureConfiguration)
    , composedSensors_(new (std::nothrow) SeaLevelPressureSensor[MaxSensorSlotCount]) {
}

bool SensorRuntime::construct(
    SensorFactory& factory,
    const SensorSlotConfiguration* slots,
    ISensor** sensors,
    size_t& activeSensorCount,
    const char*& failureReason) {
    activeSensorCount = 0;
    if (!composedSensors_ || !activePressure_) {
        failureReason = "Pressure composition allocation failed";
        return false;
    }
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
        composedSensors_[index].bind(*sensors[index], activePressure_->position,
            activePressure_->reduction, measurements_, clock_);
        const SensorRegistrationResult registration = sensorManager_.registerSensor(
            composedSensors_[index],
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
    if (!activePressure_) { failureReason = "Pressure configuration allocation failed"; return false; }
    activePressure_->position = configurationService_.getConfiguration().geoPosition;
    activePressure_->reduction = configurationService_.getConfiguration().pressureReduction;
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
    activePressure_->position = configurationService_.getConfiguration().geoPosition;
    activePressure_->reduction = configurationService_.getConfiguration().pressureReduction;
    sensorManager_.begin();
    SensorFactory* previousFactory = activeFactory_;
    activeFactory_ = inactiveFactory_;
    inactiveFactory_ = previousFactory;
    snapshotActiveConfiguration(desired, stagedSensors);
    return true;
}

} // namespace EnvNode
