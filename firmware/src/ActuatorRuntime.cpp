#include "ActuatorRuntime.h"

#include <cstring>

namespace EnvNode {

ActuatorRuntime::ActuatorRuntime(ActuatorFactory& factory, ILogger& logger)
    : factory_(factory)
    , logger_(logger) {
}

void ActuatorRuntime::initialize(const ActuatorSlotConfiguration* slots) {
    if (initialized_ || slots == nullptr) return;
    initialized_ = true;

    for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
        const ActuatorSlotConfiguration& slot = slots[slotIndex];
        if (!slot.enabled || slot.implementation == ActuatorImplementation::None) continue;

        RuntimeEntry& entry = entries_[runtimeCount_++];
        entry.info.id = slot.slotId;
        strncpy(entry.info.name, slot.name.c_str(), MaxActuatorSlotNameLength);
        entry.info.name[MaxActuatorSlotNameLength] = '\0';
        entry.info.implementation = slot.implementation;
        entry.info.hardware = slot.hardware;

        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(slot.implementation);
        if (metadata != nullptr) entry.info.capabilities = metadata->capabilities;

        ActuatorFactoryResult factoryResult;
        const ActuatorFactoryInstance instance = factory_.create(
            slotIndex, slot, factoryResult);
        entry.info.constructionResult = factoryResult;
        entry.onOff = instance.onOff;
        if (factoryResult != ActuatorFactoryResult::Created || entry.onOff == nullptr) {
            logger_.printf(
                "Actuator %u construction failed: result=%u\n",
                static_cast<unsigned int>(slot.slotId),
                static_cast<unsigned int>(factoryResult));
            continue;
        }

        entry.info.initializationAttempted = true;
        entry.info.initializationResult = entry.onOff->begin();
        if (entry.info.initializationResult != ActuatorOperationResult::Completed) {
            logger_.printf(
                "Actuator %u initialization failed: result=%u\n",
                static_cast<unsigned int>(slot.slotId),
                static_cast<unsigned int>(entry.info.initializationResult));
            continue;
        }

        entry.info.available = true;
        ++availableCount_;
        logger_.printf(
            "Actuator %u initialized successfully: %s\n",
            static_cast<unsigned int>(slot.slotId),
            slot.name.c_str());
    }
}

size_t ActuatorRuntime::runtimeCount() const {
    return runtimeCount_;
}

size_t ActuatorRuntime::availableCount() const {
    return availableCount_;
}

bool ActuatorRuntime::runtimeInfo(size_t index, ActuatorRuntimeInfo& info) const {
    if (index >= runtimeCount_) return false;
    info = entries_[index].info;
    return true;
}

IOnOffActuator* ActuatorRuntime::onOffActuator(ActuatorId id) {
    RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::OnOff)
        ? entry->onOff : nullptr;
}

const IOnOffActuator* ActuatorRuntime::onOffActuator(ActuatorId id) const {
    const RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::OnOff)
        ? entry->onOff : nullptr;
}

ActuatorRuntime::RuntimeEntry* ActuatorRuntime::findEntry(ActuatorId id) {
    for (size_t index = 0; index < runtimeCount_; ++index) {
        if (entries_[index].info.id == id) return &entries_[index];
    }
    return nullptr;
}

const ActuatorRuntime::RuntimeEntry* ActuatorRuntime::findEntry(ActuatorId id) const {
    for (size_t index = 0; index < runtimeCount_; ++index) {
        if (entries_[index].info.id == id) return &entries_[index];
    }
    return nullptr;
}

} // namespace EnvNode
