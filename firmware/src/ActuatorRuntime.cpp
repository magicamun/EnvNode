#include "ActuatorRuntime.h"

#include <cstring>
#include <new>

namespace EnvNode {

ActuatorRuntime::ActuatorRuntime(ActuatorFactory& factory, ILogger& logger)
    : activeFactory_(&factory)
    , inactiveFactory_(&secondaryFactory_)
    , secondaryFactory_(logger)
    , logger_(logger) {
    automaticActuators_ = new (std::nothrow)
        AutomaticActuatorDefinition[MaxActuatorSlotCount];
    effectiveSlots_ = new (std::nothrow)
        ActuatorSlotConfiguration[MaxActuatorSlotCount];
}

ActuatorRuntime::~ActuatorRuntime() {
    shutdownComposition(entries_, runtimeCount_);
    activeFactory_->destroyAll();
    inactiveFactory_->destroyAll();
    delete[] automaticActuators_;
    delete[] effectiveSlots_;
}

bool ActuatorRuntime::configureAutomaticActuators(
    const AutomaticActuatorDefinition* definitions,
    size_t count) {
    if (initialized_ || automaticActuators_ == nullptr
        || count > MaxActuatorSlotCount
        || (definitions == nullptr && count != 0)) {
        return false;
    }
    automaticActuatorCount_ = count;
    for (size_t index = 0; index < count; ++index) {
        automaticActuators_[index] = definitions[index];
    }
    return true;
}

bool ActuatorRuntime::moduleOwnsHardware(const HardwareResourceAssignment& hardware) const {
    for (size_t index = 0; index < automaticActuatorCount_; ++index) {
        if (exclusiveHardwareResourceConflict(automaticActuators_[index].hardware, hardware))
            return true;
    }
    return false;
}

void ActuatorRuntime::initialize(const ActuatorSlotConfiguration* slots) {
    if (initialized_ || slots == nullptr) return;
    initialized_ = true;
    if (!composeEffectiveSlots(slots)) {
        logger_.error("Actuator runtime initialization rejected: invalid effective composition");
        return;
    }
    constructComposition(*activeFactory_, effectiveSlots_, effectiveOrigins_, entries_, runtimeCount_);
    initializeComposition(entries_, runtimeCount_, availableCount_);
}

bool ActuatorRuntime::rebuild(const ActuatorSlotConfiguration* slots) {
    if (!initialized_ || !composeEffectiveSlots(slots)
        || !validateComposition(effectiveSlots_)) {
        logger_.error("Actuator runtime rebuild rejected: invalid composition");
        return false;
    }

    inactiveFactory_->destroyAll();
    RuntimeEntry stagedEntries[MaxActuatorSlotCount];
    size_t stagedRuntimeCount = 0;
    if (!constructComposition(
            *inactiveFactory_, effectiveSlots_, effectiveOrigins_, stagedEntries, stagedRuntimeCount)) {
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

bool ActuatorRuntime::composeEffectiveSlots(const ActuatorSlotConfiguration* slots) {
    if (slots == nullptr || effectiveSlots_ == nullptr
        || automaticActuators_ == nullptr) return false;
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        effectiveSlots_[index] = slots[index];
        effectiveOrigins_[index] = nullptr;
        if (validModuleActuatorReference(slots[index].moduleTarget)) {
            effectiveSlots_[index].enabled = false;
            effectiveSlots_[index].implementation = ActuatorImplementation::None;
            effectiveSlots_[index].hardware = HardwareResourceAssignment::none();
        }
    }

    for (size_t automaticIndex = 0;
         automaticIndex < automaticActuatorCount_;
         ++automaticIndex) {
        const AutomaticActuatorDefinition& automatic =
            automaticActuators_[automaticIndex];
        for (size_t previous = 0; previous < automaticIndex; ++previous) {
            if (exclusiveHardwareResourceConflict(
                    automaticActuators_[previous].hardware, automatic.hardware)) {
                logger_.errorf("Duplicate module actuator resource for device %s",
                    automatic.deviceId);
                return false;
            }
        }
        size_t target = MaxActuatorSlotCount;
        ModuleActuatorReference reference;
        if (automatic.hasModuleInstanceId) {
            memcpy(reference.moduleInstanceFingerprint, automatic.moduleInstanceFingerprint, 8);
            setModuleActuatorDeviceId(reference, automatic.deviceId);
        }
        bool configuredModule = false;
        for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
            if (sameModuleActuatorReference(slots[slotIndex].moduleTarget, reference)) {
                target = slotIndex;
                configuredModule = true;
                break;
            }
        }
        if (configuredModule) {
            for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
                if (index != target && slots[index].enabled
                    && slots[index].implementation != ActuatorImplementation::None
                    && exclusiveHardwareResourceConflict(slots[index].hardware, automatic.hardware)) {
                    logger_.error("Manual actuator conflicts with configured module hardware");
                    return false;
                }
            }
            if (!slots[target].enabled) continue;
        }
        if (!configuredModule) {
            for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
                const ActuatorSlotConfiguration& slot = effectiveSlots_[slotIndex];
                if (!validModuleActuatorReference(slots[slotIndex].moduleTarget)
                    && slot.enabled && slot.implementation != ActuatorImplementation::None
                    && exclusiveHardwareResourceConflict(slot.hardware, automatic.hardware)) {
                    target = slotIndex;
                    break;
                }
            }
        }
        if (target == MaxActuatorSlotCount) {
            for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
                if (!validModuleActuatorReference(slots[slotIndex].moduleTarget)
                    && effectiveSlots_[slotIndex].implementation == ActuatorImplementation::None) {
                    target = slotIndex;
                    break;
                }
            }
        }
        if (target == MaxActuatorSlotCount) {
            logger_.errorf("No runtime slot available for module actuator %s",
                automatic.deviceId);
            return false;
        }

        ActuatorSlotConfiguration& effective = effectiveSlots_[target];
        effective.enabled = true;
        effective.name = configuredModule ? slots[target].name : String(automatic.name);
        effective.implementation = automatic.implementation;
        effective.hardware = automatic.hardware;
        effectiveOrigins_[target] = &automaticActuators_[automaticIndex];
        logger_.infof(
            "Module actuator slot=%s device=%s assigned runtime actuator=%u GPIO=%u",
            moduleSlotName(automatic.moduleSlot), automatic.deviceId,
            static_cast<unsigned int>(effective.slotId),
            static_cast<unsigned int>(effective.hardware.gpio.number));
    }
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
    const AutomaticActuatorDefinition* const* origins,
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
        if (origins != nullptr && origins[slotIndex] != nullptr) {
            const AutomaticActuatorDefinition& origin = *origins[slotIndex];
            entry.info.origin = ActuatorRuntimeOrigin::ModuleDescriptor;
            entry.info.moduleSlot = origin.moduleSlot;
            entry.automaticOrigin = origins[slotIndex];
            strncpy(entry.info.descriptorDeviceId,
                origin.deviceId, MaxActuatorSlotNameLength);
            entry.info.descriptorDeviceId[MaxActuatorSlotNameLength] = '\0';
        }
        const ActuatorImplementationMetadata* metadata =
            ActuatorImplementationRegistry::find(slot.implementation);
        if (metadata != nullptr) entry.info.capabilities = metadata->capabilities;

        ActuatorFactoryResult factoryResult;
        const ActuatorFactoryInstance instance = factory.create(
            slotIndex, slot, factoryResult);
        entry.info.constructionResult = factoryResult;
        entry.onOff = instance.onOff;
        entry.level = instance.level;
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

bool ActuatorRuntime::moduleReference(
    ActuatorId id, ModuleActuatorReference& reference) const {
    const RuntimeEntry* entry = findEntry(id);
    if (entry == nullptr || entry->automaticOrigin == nullptr
        || !entry->automaticOrigin->hasModuleInstanceId) return false;
    reference = ModuleActuatorReference{};
    setModuleActuatorInstanceId(reference,
        entry->automaticOrigin->moduleInstanceFingerprint);
    setModuleActuatorDeviceId(reference, entry->automaticOrigin->deviceId);
    return validModuleActuatorReference(reference);
}

IOnOffActuator* ActuatorRuntime::onOffActuator(ActuatorId id) {
    RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::OnOff)
        ? entry->onOff : nullptr;
}

IOnOffActuator* ActuatorRuntime::onOffActuator(
    const ModuleActuatorReference& reference) {
    if (!validModuleActuatorReference(reference)) return nullptr;
    for (size_t index = 0; index < runtimeCount_; ++index) {
        RuntimeEntry& entry = entries_[index];
        ModuleActuatorReference runtimeReference;
        if (!moduleReference(entry.info.id, runtimeReference)
            || !sameModuleActuatorReference(runtimeReference, reference)) continue;
        return entry.info.available
            && hasActuatorCapability(entry.info.capabilities, ActuatorCapability::OnOff)
            ? entry.onOff : nullptr;
    }
    return nullptr;
}

const IOnOffActuator* ActuatorRuntime::onOffActuator(ActuatorId id) const {
    const RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::OnOff)
        ? entry->onOff : nullptr;
}

ILevelActuator* ActuatorRuntime::levelActuator(ActuatorId id) {
    RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::Level)
        ? entry->level : nullptr;
}

const ILevelActuator* ActuatorRuntime::levelActuator(ActuatorId id) const {
    const RuntimeEntry* entry = findEntry(id);
    return entry != nullptr && entry->info.available
        && hasActuatorCapability(entry->info.capabilities, ActuatorCapability::Level)
        ? entry->level : nullptr;
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
