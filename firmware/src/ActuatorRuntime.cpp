#include "ActuatorRuntime.h"

#include <cstring>

namespace EnvNode {

ActuatorRuntime::ActuatorRuntime(ActuatorFactory& factory, ILogger& logger)
    : activeFactory_(&factory)
    , inactiveFactory_(&secondaryFactory_)
    , secondaryFactory_(logger)
    , logger_(logger) {
}

ActuatorRuntime::~ActuatorRuntime() {
    shutdownComposition(entries_, runtimeCount_);
    activeFactory_->destroyAll();
    inactiveFactory_->destroyAll();
}

void ActuatorRuntime::initialize(const ActuatorSlotConfiguration* slots) {
    if (initialized_ || slots == nullptr) return;
    initialized_ = true;
    constructComposition(*activeFactory_, slots, entries_, runtimeCount_);
    initializeComposition(entries_, runtimeCount_, availableCount_);
}

bool ActuatorRuntime::rebuild(const ActuatorSlotConfiguration* slots) {
    if (!initialized_ || !validateComposition(slots)) {
        logger_.error("Actuator runtime rebuild rejected: invalid composition");
        return false;
    }

    inactiveFactory_->destroyAll();
    RuntimeEntry stagedEntries[MaxActuatorSlotCount];
    size_t stagedRuntimeCount = 0;
    if (!constructComposition(
            *inactiveFactory_, slots, stagedEntries, stagedRuntimeCount)) {
        inactiveFactory_->destroyAll();
        logger_.error("Actuator runtime rebuild failed during staging");
        return false;
    }

    shutdownComposition(entries_, runtimeCount_);
    size_t stagedAvailableCount = 0;
    if (!initializeComposition(
            stagedEntries, stagedRuntimeCount, stagedAvailableCount)) {
        shutdownComposition(stagedEntries, stagedRuntimeCount);
        inactiveFactory_->destroyAll();

        availableCount_ = 0;
        const bool rollbackSucceeded = initializeComposition(
            entries_, runtimeCount_, availableCount_);
        if (rollbackSucceeded) {
            logger_.error("Actuator runtime rebuild failed; previous composition restored");
        } else {
            logger_.error(
                "Actuator runtime rebuild failed; previous composition rollback incomplete");
        }
        return false;
    }

    activeFactory_->destroyAll();
    ActuatorFactory* previousFactory = activeFactory_;
    activeFactory_ = inactiveFactory_;
    inactiveFactory_ = previousFactory;
    memcpy(entries_, stagedEntries, sizeof(entries_));
    runtimeCount_ = stagedRuntimeCount;
    availableCount_ = stagedAvailableCount;
    logger_.infof("Actuator runtime rebuild successful: %u actuators active",
        static_cast<unsigned int>(availableCount_));
    return true;
}

bool ActuatorRuntime::validateComposition(const ActuatorSlotConfiguration* slots) const {
    if (slots == nullptr) return false;
    HardwareResourceClaim claims[MaxActuatorSlotCount];
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        const ActuatorSlotConfiguration& slot = slots[index];
        if (slot.slotId != index + 1
            || slot.name.isEmpty()
            || slot.name.length() > MaxActuatorSlotNameLength) {
            return false;
        }
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(slot.implementation);
        if (metadata == nullptr) return false;
        if (slot.implementation == ActuatorImplementation::None) {
            if (slot.hardware.kind != HardwareResourceKind::None) return false;
        } else if (BoardCapabilities::current().validate(
                metadata->interfaceKind,
                slot.hardware,
                metadata->requiredGpioCapabilities)
                != HardwareResourceValidationResult::Valid) {
            return false;
        }
        const bool active = slot.enabled
            && slot.implementation != ActuatorImplementation::None;
        claims[index] = {active, slot.hardware};
    }
    return validateExclusiveHardwareResourceOccupancy(
        claims, MaxActuatorSlotCount);
}

bool ActuatorRuntime::constructComposition(
    ActuatorFactory& factory,
    const ActuatorSlotConfiguration* slots,
    RuntimeEntry* entries,
    size_t& runtimeCount) const {
    runtimeCount = 0;
    bool success = true;
    for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
        const ActuatorSlotConfiguration& slot = slots[slotIndex];
        if (!slot.enabled || slot.implementation == ActuatorImplementation::None) continue;

        RuntimeEntry& entry = entries[runtimeCount++];
        entry = RuntimeEntry{};
        entry.info.id = slot.slotId;
        strncpy(entry.info.name, slot.name.c_str(), MaxActuatorSlotNameLength);
        entry.info.name[MaxActuatorSlotNameLength] = '\0';
        entry.info.implementation = slot.implementation;
        entry.info.hardware = slot.hardware;
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(slot.implementation);
        if (metadata != nullptr) entry.info.capabilities = metadata->capabilities;

        ActuatorFactoryResult factoryResult;
        const ActuatorFactoryInstance instance = factory.create(
            slotIndex, slot, factoryResult);
        entry.info.constructionResult = factoryResult;
        entry.onOff = instance.onOff;
        if (factoryResult != ActuatorFactoryResult::Created || entry.onOff == nullptr) {
            success = false;
            logger_.errorf("Actuator %u construction failed: result=%u",
                static_cast<unsigned int>(slot.slotId),
                static_cast<unsigned int>(factoryResult));
        }
    }
    return success;
}

bool ActuatorRuntime::initializeComposition(
    RuntimeEntry* entries,
    size_t runtimeCount,
    size_t& availableCount) const {
    availableCount = 0;
    bool success = true;
    for (size_t index = 0; index < runtimeCount; ++index) {
        RuntimeEntry& entry = entries[index];
        entry.info.available = false;
        entry.info.initializationAttempted = entry.onOff != nullptr;
        if (entry.onOff == nullptr) {
            success = false;
            continue;
        }
        entry.info.initializationResult = entry.onOff->begin();
        if (entry.info.initializationResult != ActuatorOperationResult::Completed) {
            success = false;
            logger_.errorf("Actuator %u initialization failed: result=%u",
                static_cast<unsigned int>(entry.info.id),
                static_cast<unsigned int>(entry.info.initializationResult));
            continue;
        }
        entry.info.available = true;
        ++availableCount;
        logger_.infof("Actuator %u initialized successfully: %s",
            static_cast<unsigned int>(entry.info.id), entry.info.name);
    }
    return success;
}

void ActuatorRuntime::shutdownComposition(
    RuntimeEntry* entries,
    size_t runtimeCount) const {
    for (size_t index = 0; index < runtimeCount; ++index) {
        RuntimeEntry& entry = entries[index];
        if (entry.onOff != nullptr) entry.onOff->shutdown();
        entry.info.available = false;
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
