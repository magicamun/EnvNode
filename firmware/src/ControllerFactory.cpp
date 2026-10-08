#include "ControllerFactory.h"

#include <climits>
#include <cmath>
#include <new>

namespace EnvNode {

ControllerFactory::ControllerFactory(
    IMeasurementResolver& measurementResolver,
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger, const IEnumValueReader* values)
    : measurementResolver_(measurementResolver)
    , actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger), values_(values) {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        constructed_[index] = ControllerImplementation::None;
    }
}

ControllerFactory::~ControllerFactory() { destroyAll(); }

ControllerFactoryInstance ControllerFactory::create(
    size_t storageIndex,
    const ControllerSlotConfiguration& slot,
    ControllerFactoryResult& result) {
    ControllerFactoryInstance instance;
    if (storageIndex >= MaxControllerSlotCount) {
        result = ControllerFactoryResult::InvalidStorageIndex;
        return instance;
    }
    if (constructed_[storageIndex] != ControllerImplementation::None) {
        result = ControllerFactoryResult::StorageOccupied;
        return instance;
    }
    if (!slot.enabled || slot.implementation == ControllerImplementation::None) {
        result = ControllerFactoryResult::NoRuntimeController;
        return instance;
    }
    if (ControllerImplementationRegistry::find(slot.implementation) == nullptr) {
        result = ControllerFactoryResult::UnknownImplementation;
        return instance;
    }
    void* target = &storage_[storageIndex];
    if (slot.implementation == ControllerImplementation::Blink) {
        const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
        if ((!validModuleActuatorReference(slot.moduleTarget)
                && !isValidActuatorId(blink.targetActuatorId))
            || blink.onDurationMs == 0 || blink.onDurationMs > INT32_MAX
            || blink.offDurationMs == 0 || blink.offDurationMs > INT32_MAX) {
            result = ControllerFactoryResult::InvalidConfiguration;
            return instance;
        }
        instance.blink = new (target) BlinkController(
            blink, slot.moduleTarget, actuatorResolver_, monotonicClock_, logger_,
            slot.slotId, slot.name);
        instance.controller = instance.blink;
        constructed_[storageIndex] = ControllerImplementation::Blink;
    } else if (slot.implementation == ControllerImplementation::Threshold) {
        const ThresholdControllerConfiguration& threshold =
            slot.implementationConfiguration.threshold;
        const MeasurementTypeMetadata& measurementMetadata =
            measurementTypeMetadata(threshold.source.measurementType);
        if (!isValidSensorId(threshold.source.sensorId)
            || measurementMetadata.expectedValueKind != ValueKind::FloatingPoint
            || measurementMetadata.semantics != MeasurementSemantics::State
            || (!threshold.decisionOnly && !validModuleActuatorReference(slot.moduleTarget)
                && !isValidActuatorId(threshold.targetActuatorId))
            || !std::isfinite(threshold.onThreshold)
            || !std::isfinite(threshold.offThreshold)
            || !validThresholdOrdering(threshold.direction,
                threshold.onThreshold, threshold.offThreshold)
            || threshold.maxMeasurementAgeMs == 0
            || threshold.maxMeasurementAgeMs > INT32_MAX) {
            result = ControllerFactoryResult::InvalidConfiguration;
            return instance;
        }
        instance.threshold = new (target) ThresholdController(
            threshold, slot.moduleTarget,
            measurementResolver_, actuatorResolver_, monotonicClock_, logger_,
            slot.slotId, slot.name);
        instance.controller = instance.threshold;
        constructed_[storageIndex] = ControllerImplementation::Threshold;
    } else if (slot.implementation == ControllerImplementation::Selector) {
        const auto& selector = slot.implementationConfiguration.selector;
        const auto* definition = values_ ? values_->valueDefinition(selector.modeValueId) : nullptr;
        if (!definition || !validSelectorMapping(selector, *definition)
            || (!validModuleActuatorReference(slot.moduleTarget) && !isValidActuatorId(selector.targetActuatorId))) {
            result = ControllerFactoryResult::InvalidConfiguration;
            return instance;
        }
        instance.selector = new (target) SelectorController(selector, slot.moduleTarget, *values_, actuatorResolver_);
        instance.controller = instance.selector;
        constructed_[storageIndex] = ControllerImplementation::Selector;
    } else {
        result = ControllerFactoryResult::UnknownImplementation;
        return instance;
    }
    result = ControllerFactoryResult::Created;
    return instance;
}

void ControllerFactory::destroy(size_t storageIndex) {
    if (constructed_[storageIndex] == ControllerImplementation::Blink) {
        static_cast<BlinkController*>(
            static_cast<void*>(&storage_[storageIndex]))->~BlinkController();
    } else if (constructed_[storageIndex] == ControllerImplementation::Threshold) {
        static_cast<ThresholdController*>(
            static_cast<void*>(&storage_[storageIndex]))->~ThresholdController();
    }
    if (constructed_[storageIndex] == ControllerImplementation::Selector) {
        static_cast<SelectorController*>(static_cast<void*>(&storage_[storageIndex]))->~SelectorController();
    }
    constructed_[storageIndex] = ControllerImplementation::None;
}

void ControllerFactory::destroyAll() {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) destroy(index);
}

IMeasurementResolver& ControllerFactory::measurementResolver() const {
    return measurementResolver_;
}

IOnOffActuatorResolver& ControllerFactory::actuatorResolver() const {
    return actuatorResolver_;
}

IMonotonicClock& ControllerFactory::monotonicClock() const {
    return monotonicClock_;
}

} // namespace EnvNode
