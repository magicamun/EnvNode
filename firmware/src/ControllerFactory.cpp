#include "ControllerFactory.h"

#include <climits>
#include <new>

namespace EnvNode {

ControllerFactory::ControllerFactory(
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger)
    : actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger) {
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
    const BlinkControllerConfiguration& blink = slot.implementationConfiguration.blink;
    if (slot.implementation != ControllerImplementation::Blink
        || !isValidActuatorId(blink.targetActuatorId)
        || blink.onDurationMs == 0 || blink.onDurationMs > INT32_MAX
        || blink.offDurationMs == 0 || blink.offDurationMs > INT32_MAX) {
        result = ControllerFactoryResult::InvalidConfiguration;
        return instance;
    }

    void* target = &storage_[storageIndex];
    instance.blink = new (target) BlinkController(
        blink, actuatorResolver_, monotonicClock_, logger_);
    instance.controller = instance.blink;
    constructed_[storageIndex] = ControllerImplementation::Blink;
    result = ControllerFactoryResult::Created;
    return instance;
}

void ControllerFactory::destroy(size_t storageIndex) {
    if (constructed_[storageIndex] == ControllerImplementation::Blink) {
        static_cast<BlinkController*>(
            static_cast<void*>(&storage_[storageIndex]))->~BlinkController();
    }
    constructed_[storageIndex] = ControllerImplementation::None;
}

void ControllerFactory::destroyAll() {
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) destroy(index);
}

IOnOffActuatorResolver& ControllerFactory::actuatorResolver() const {
    return actuatorResolver_;
}

IMonotonicClock& ControllerFactory::monotonicClock() const {
    return monotonicClock_;
}

} // namespace EnvNode
