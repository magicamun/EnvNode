#include "ControllerRuntime.h"

#include <climits>
#include <cmath>
#include <cstring>

namespace EnvNode {

ControllerRuntime::ControllerRuntime(ControllerFactory& factory, ILogger& logger)
    : activeFactory_(&factory)
    , inactiveFactory_(&secondaryFactory_)
    , secondaryFactory_(
        factory.measurementResolver(), factory.actuatorResolver(),
        factory.monotonicClock(), logger)
    , logger_(logger) {
}

ControllerRuntime::~ControllerRuntime() {
    stopComposition(entries_, runtimeCount_);
    activeFactory_->destroyAll();
    inactiveFactory_->destroyAll();
}

bool ControllerRuntime::initialize(const ControllerSlotConfiguration* slots) {
    if (initialized_ || !validateComposition(slots)) return false;
    initialized_ = true;
    if (!constructComposition(*activeFactory_, slots, entries_, runtimeCount_)) {
        activeFactory_->destroyAll();
        runtimeCount_ = 0;
        return false;
    }
    if (!beginComposition(entries_, runtimeCount_)) {
        stopComposition(entries_, runtimeCount_);
        activeFactory_->destroyAll();
        runtimeCount_ = 0;
        return false;
    }
    ++compositionRevision_;
    return true;
}

bool ControllerRuntime::rebuild(const ControllerSlotConfiguration* slots) {
    if (!initialized_ || !validateComposition(slots)) {
        logger_.error("Controller runtime rebuild rejected: invalid composition");
        return false;
    }

    inactiveFactory_->destroyAll();
    RuntimeEntry stagedEntries[MaxControllerSlotCount];
    size_t stagedRuntimeCount = 0;
    if (!constructComposition(
            *inactiveFactory_, slots, stagedEntries, stagedRuntimeCount)) {
        inactiveFactory_->destroyAll();
        logger_.error("Controller runtime rebuild failed during staging");
        return false;
    }

    stopComposition(entries_, runtimeCount_);
    if (!beginComposition(stagedEntries, stagedRuntimeCount)) {
        stopComposition(stagedEntries, stagedRuntimeCount);
        inactiveFactory_->destroyAll();
        const bool restored = beginComposition(entries_, runtimeCount_);
        if (restored) {
            logger_.error("Controller runtime rebuild failed; previous composition restored");
        } else {
            logger_.error(
                "Controller runtime rebuild failed; previous composition rollback incomplete");
        }
        return false;
    }

    activeFactory_->destroyAll();
    ControllerFactory* previousFactory = activeFactory_;
    activeFactory_ = inactiveFactory_;
    inactiveFactory_ = previousFactory;
    memcpy(entries_, stagedEntries, sizeof(entries_));
    runtimeCount_ = stagedRuntimeCount;
    ++compositionRevision_;
    logger_.infof("Controller runtime rebuild successful: %u controllers active",
        static_cast<unsigned int>(runtimeCount_));
    return true;
}

void ControllerRuntime::loop() {
    for (size_t index = 0; index < runtimeCount_; ++index) {
        RuntimeEntry& entry = entries_[index];
        if (entry.controller == nullptr) continue;
        const ControllerOperationResult result = entry.controller->service();
        if (result != ControllerOperationResult::NoAction) {
            entry.info.lastOperationResult = result;
        }
        updateRuntimeInfo(entry);
    }
}

size_t ControllerRuntime::runtimeCount() const { return runtimeCount_; }
uint32_t ControllerRuntime::compositionRevision() const { return compositionRevision_; }

bool ControllerRuntime::runtimeInfo(size_t index, ControllerRuntimeInfo& info) const {
    if (index >= runtimeCount_) return false;
    info = entries_[index].info;
    return true;
}

ControllerOperationResult ControllerRuntime::startController(ControllerId id) {
    RuntimeEntry* entry = findEntry(id);
    if (entry == nullptr || entry->controller == nullptr) {
        return ControllerOperationResult::ControllerNotFound;
    }
    if (entry->info.running) return ControllerOperationResult::NoAction;
    entry->info.initializationAttempted = true;
    entry->info.lastOperationResult = entry->controller->begin();
    updateRuntimeInfo(*entry);
    return entry->info.lastOperationResult;
}

ControllerOperationResult ControllerRuntime::stopController(ControllerId id) {
    RuntimeEntry* entry = findEntry(id);
    if (entry == nullptr || entry->controller == nullptr) {
        return ControllerOperationResult::ControllerNotFound;
    }
    entry->info.lastOperationResult = entry->controller->stop();
    updateRuntimeInfo(*entry);
    return entry->info.lastOperationResult;
}

bool ControllerRuntime::validateComposition(
    const ControllerSlotConfiguration* slots) const {
    if (slots == nullptr) return false;
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        const ControllerSlotConfiguration& slot = slots[index];
        if (slot.slotId != index + 1
            || slot.name.isEmpty()
            || slot.name.length() > MaxControllerSlotNameLength
            || ControllerImplementationRegistry::find(slot.implementation) == nullptr) {
            return false;
        }
        if (!slot.enabled || slot.implementation == ControllerImplementation::None) continue;
        if (slot.implementation == ControllerImplementation::Blink) {
            const BlinkControllerConfiguration& blink =
                slot.implementationConfiguration.blink;
            if (!isValidActuatorId(blink.targetActuatorId)
                || blink.onDurationMs == 0 || blink.onDurationMs > INT32_MAX
                || blink.offDurationMs == 0 || blink.offDurationMs > INT32_MAX) {
                return false;
            }
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const ThresholdControllerConfiguration& threshold =
                slot.implementationConfiguration.threshold;
            const MeasurementTypeMetadata& metadata =
                measurementTypeMetadata(threshold.source.measurementType);
            if (!isValidSensorId(threshold.source.sensorId)
                || metadata.expectedValueKind != ValueKind::FloatingPoint
                || metadata.semantics != MeasurementSemantics::State
                || !isValidActuatorId(threshold.targetActuatorId)
                || !std::isfinite(threshold.onThreshold)
                || !std::isfinite(threshold.offThreshold)
                || !validThresholdOrdering(threshold.direction,
                    threshold.onThreshold, threshold.offThreshold)
                || threshold.maxMeasurementAgeMs == 0
                || threshold.maxMeasurementAgeMs > INT32_MAX) {
                return false;
            }
        } else {
            return false;
        }
    }
    return true;
}

bool ControllerRuntime::constructComposition(
    ControllerFactory& factory,
    const ControllerSlotConfiguration* slots,
    RuntimeEntry* entries,
    size_t& runtimeCount) const {
    runtimeCount = 0;
    for (size_t slotIndex = 0; slotIndex < MaxControllerSlotCount; ++slotIndex) {
        const ControllerSlotConfiguration& slot = slots[slotIndex];
        if (!slot.enabled || slot.implementation == ControllerImplementation::None) continue;
        RuntimeEntry& entry = entries[runtimeCount++];
        entry = RuntimeEntry{};
        entry.info.id = slot.slotId;
        strncpy(entry.info.name, slot.name.c_str(), MaxControllerSlotNameLength);
        entry.info.name[MaxControllerSlotNameLength] = '\0';
        entry.info.implementation = slot.implementation;
        if (slot.implementation == ControllerImplementation::Blink) {
            const BlinkControllerConfiguration& blink =
                slot.implementationConfiguration.blink;
            entry.info.targetActuatorId = blink.targetActuatorId;
            entry.info.onDurationMs = blink.onDurationMs;
            entry.info.offDurationMs = blink.offDurationMs;
        } else if (slot.implementation == ControllerImplementation::Threshold) {
            const ThresholdControllerConfiguration& threshold =
                slot.implementationConfiguration.threshold;
            entry.info.targetActuatorId = threshold.targetActuatorId;
            entry.info.sourceSensorId = threshold.source.sensorId;
            entry.info.sourceMeasurementType = threshold.source.measurementType;
            entry.info.onThreshold = threshold.onThreshold;
            entry.info.offThreshold = threshold.offThreshold;
            entry.info.thresholdDirection = threshold.direction;
            entry.info.maxMeasurementAgeMs = threshold.maxMeasurementAgeMs;
        }
        const ControllerFactoryInstance instance = factory.create(
            slotIndex, slot, entry.info.constructionResult);
        entry.controller = instance.controller;
        entry.blink = instance.blink;
        entry.threshold = instance.threshold;
        if (entry.info.constructionResult != ControllerFactoryResult::Created
            || entry.controller == nullptr) {
            logger_.errorf("Controller %u construction failed: result=%u",
                static_cast<unsigned int>(slot.slotId),
                static_cast<unsigned int>(entry.info.constructionResult));
            return false;
        }
    }
    return true;
}

bool ControllerRuntime::beginComposition(
    RuntimeEntry* entries,
    size_t runtimeCount) const {
    for (size_t index = 0; index < runtimeCount; ++index) {
        RuntimeEntry& entry = entries[index];
        entry.info.initializationAttempted = true;
        entry.info.lastOperationResult = entry.controller->begin();
        updateRuntimeInfo(entry);
        if (entry.info.lastOperationResult == ControllerOperationResult::InvalidConfiguration) {
            return false;
        }
        logger_.infof("Controller %u activated: %s",
            static_cast<unsigned int>(entry.info.id), entry.info.name);
    }
    return true;
}

void ControllerRuntime::stopComposition(
    RuntimeEntry* entries,
    size_t runtimeCount) const {
    for (size_t index = 0; index < runtimeCount; ++index) {
        RuntimeEntry& entry = entries[index];
        if (entry.controller != nullptr) {
            entry.info.lastOperationResult = entry.controller->stop();
            updateRuntimeInfo(entry);
        }
    }
}

void ControllerRuntime::updateRuntimeInfo(RuntimeEntry& entry) {
    if (entry.blink != nullptr) {
        entry.info.running = entry.blink->running();
        entry.info.targetAvailable = entry.blink->targetAvailable();
        entry.info.blinkPhase = entry.blink->phase();
    } else if (entry.threshold != nullptr) {
        entry.info.running = entry.threshold->running();
        entry.info.targetAvailable = entry.threshold->targetAvailable();
        entry.info.sourceAvailable = entry.threshold->sourceAvailable();
        entry.info.hasLatestSnapshot = entry.threshold->hasLatestSnapshot();
        entry.info.latestMeasurementValid =
            entry.threshold->latestMeasurementValid();
        entry.info.latestNumericValueAvailable =
            entry.threshold->latestNumericValueAvailable();
        entry.info.latestNumericValue = entry.threshold->latestNumericValue();
        entry.info.latestSnapshotStale = entry.threshold->latestSnapshotStale();
        entry.info.latestSnapshotAgeMs = entry.threshold->latestSnapshotAgeMs();
        entry.info.thresholdDecision = entry.threshold->decision();
        entry.info.outputApplicationPending =
            entry.threshold->outputApplicationPending();
    }
}

ControllerRuntime::RuntimeEntry* ControllerRuntime::findEntry(ControllerId id) {
    for (size_t index = 0; index < runtimeCount_; ++index) {
        if (entries_[index].info.id == id) return &entries_[index];
    }
    return nullptr;
}

} // namespace EnvNode
