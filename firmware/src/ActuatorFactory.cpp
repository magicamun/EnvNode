#include "ActuatorFactory.h"

#include <new>

namespace EnvNode {

ActuatorFactory::ActuatorFactory(ILogger& logger)
    : logger_(logger) {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        constructed_[index] = ActuatorImplementation::None;
    }
}

ActuatorFactory::~ActuatorFactory() {
    destroyAll();
}

ActuatorFactoryInstance ActuatorFactory::create(
    size_t storageIndex,
    const ActuatorSlotConfiguration& slot,
    ActuatorFactoryResult& result) {
    ActuatorFactoryInstance instance;
    if (storageIndex >= MaxActuatorSlotCount) {
        result = ActuatorFactoryResult::InvalidStorageIndex;
        return instance;
    }
    if (constructed_[storageIndex] != ActuatorImplementation::None) {
        result = ActuatorFactoryResult::StorageOccupied;
        return instance;
    }
    if (!slot.enabled || slot.implementation == ActuatorImplementation::None) {
        result = ActuatorFactoryResult::NoRuntimeActuator;
        return instance;
    }

    const ActuatorImplementationMetadata* metadata =
        ActuatorImplementationRegistry::find(slot.implementation);
    if (metadata == nullptr) {
        result = ActuatorFactoryResult::UnknownImplementation;
        return instance;
    }
    if (BoardCapabilities::current().validate(
            metadata->interfaceKind,
            slot.hardware,
            metadata->requiredGpioCapabilities)
        != HardwareResourceValidationResult::Valid) {
        result = ActuatorFactoryResult::InvalidResource;
        return instance;
    }

    void* target = &storage_[storageIndex];
    switch (slot.implementation) {
        case ActuatorImplementation::GpioOnOff:
            instance.onOff = new (target) GpioOnOffActuator(slot.hardware, logger_);
            break;
        case ActuatorImplementation::None:
        default:
            result = ActuatorFactoryResult::UnknownImplementation;
            return ActuatorFactoryInstance{};
    }

    constructed_[storageIndex] = slot.implementation;
    result = ActuatorFactoryResult::Created;
    return instance;
}

void ActuatorFactory::destroy(size_t storageIndex) {
    if (constructed_[storageIndex] == ActuatorImplementation::GpioOnOff) {
        static_cast<GpioOnOffActuator*>(
            static_cast<void*>(&storage_[storageIndex]))->~GpioOnOffActuator();
    }
    constructed_[storageIndex] = ActuatorImplementation::None;
}

void ActuatorFactory::destroyAll() {
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        destroy(index);
    }
}

} // namespace EnvNode
