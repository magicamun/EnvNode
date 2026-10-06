#pragma once

#include <cstddef>

#include "ControllerFactory.h"

namespace EnvNode {

struct ControllerRuntimeInfo {
    ControllerId id = InvalidControllerId;
    char name[MaxControllerSlotNameLength + 1] = {};
    ControllerImplementation implementation = ControllerImplementation::None;
    ActuatorId targetActuatorId = InvalidActuatorId;
    uint32_t onDurationMs = 0;
    uint32_t offDurationMs = 0;
    SensorId sourceSensorId = InvalidSensorId;
    MeasurementType sourceMeasurementType = MeasurementType::Unknown;
    float onThreshold = 0.0F;
    float offThreshold = 0.0F;
    ThresholdDirection thresholdDirection = ThresholdDirection::OnAbove;
    uint32_t maxMeasurementAgeMs = 0;
    bool sourceAvailable = false;
    bool hasLatestSnapshot = false;
    bool latestMeasurementValid = false;
    bool latestNumericValueAvailable = false;
    float latestNumericValue = 0.0F;
    bool latestSnapshotStale = false;
    uint32_t latestSnapshotAgeMs = 0;
    ThresholdDecision thresholdDecision = ThresholdDecision::Unknown;
    bool outputApplicationPending = false;
    ControllerFactoryResult constructionResult =
        ControllerFactoryResult::NoRuntimeController;
    bool initializationAttempted = false;
    bool running = false;
    bool targetAvailable = false;
    BlinkPhase blinkPhase = BlinkPhase::Stopped;
    ControllerOperationResult lastOperationResult =
        ControllerOperationResult::NotRunning;
};

class ControllerRuntime {
public:
    ControllerRuntime(ControllerFactory& factory, ILogger& logger);
    ~ControllerRuntime();

    bool initialize(const ControllerSlotConfiguration* slots);
    bool rebuild(const ControllerSlotConfiguration* slots);
    void loop();
    size_t runtimeCount() const;
    // Borrowed read-only capability; valid only until runtime rebuild.
    const IThresholdReasonProvider* reasonProvider(ControllerId id) const;
    uint32_t compositionRevision() const;
    bool runtimeInfo(size_t index, ControllerRuntimeInfo& info) const;
    ControllerOperationResult startController(ControllerId id);
    ControllerOperationResult stopController(ControllerId id);

private:
    struct RuntimeEntry {
        ControllerRuntimeInfo info;
        IController* controller = nullptr;
        BlinkController* blink = nullptr;
        ThresholdController* threshold = nullptr;
    };

    bool validateComposition(const ControllerSlotConfiguration* slots) const;
    bool constructComposition(
        ControllerFactory& factory,
        const ControllerSlotConfiguration* slots,
        RuntimeEntry* entries,
        size_t& runtimeCount) const;
    bool beginComposition(RuntimeEntry* entries, size_t runtimeCount) const;
    void stopComposition(RuntimeEntry* entries, size_t runtimeCount) const;
    static void updateRuntimeInfo(RuntimeEntry& entry);
    RuntimeEntry* findEntry(ControllerId id);

    ControllerFactory* activeFactory_;
    ControllerFactory* inactiveFactory_;
    ControllerFactory secondaryFactory_;
    ILogger& logger_;
    RuntimeEntry entries_[MaxControllerSlotCount];
    size_t runtimeCount_ = 0;
    uint32_t compositionRevision_ = 0;
    bool initialized_ = false;
};

} // namespace EnvNode
