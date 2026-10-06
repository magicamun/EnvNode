#pragma once

#include "ControllerSlotConfiguration.h"
#include "IController.h"
#include "IThresholdReasonProvider.h"
#include "IMeasurementResolver.h"
#include "IMonotonicClock.h"
#include "IOnOffActuatorResolver.h"
#include "Logger.h"

namespace EnvNode {

enum class ThresholdDecision : uint8_t {
    Unknown,
    On,
    Off,
};

class ThresholdController : public IController, public IThresholdReasonProvider {
public:
    ThresholdController(
        const ThresholdControllerConfiguration& configuration,
        const ModuleActuatorReference& moduleTarget,
        IMeasurementResolver& measurementResolver,
        IOnOffActuatorResolver& actuatorResolver,
        IMonotonicClock& monotonicClock,
        ILogger& logger,
        ControllerId controllerId = InvalidControllerId,
        const String& controllerName = String());
    ThresholdController(
        const ThresholdControllerConfiguration& configuration,
        IMeasurementResolver& measurementResolver,
        IOnOffActuatorResolver& actuatorResolver,
        IMonotonicClock& monotonicClock,
        ILogger& logger,
        ControllerId controllerId = InvalidControllerId,
        const String& controllerName = String())
        : ThresholdController(configuration, ModuleActuatorReference{},
            measurementResolver, actuatorResolver, monotonicClock, logger,
            controllerId, controllerName) {}

    ControllerOperationResult begin() override;
    ControllerOperationResult service() override;
    ControllerOperationResult stop() override;

    bool running() const;
    bool sourceAvailable() const;
    bool hasLatestSnapshot() const;
    bool latestMeasurementValid() const;
    bool latestNumericValueAvailable() const;
    float latestNumericValue() const;
    bool latestSnapshotStale() const;
    uint32_t latestSnapshotAgeMs() const;
    ThresholdDecision decision() const;
    ThresholdReason reason() const override;
    bool targetAvailable() const;
    bool outputApplicationPending() const;
    bool hasProcessedRevision() const;
    uint32_t lastProcessedRevision() const;

private:
    bool configurationValid() const;
    enum class SourceStatus : uint8_t { Unknown, Available, Unavailable, Stale };
    void updateSourceAvailability(SourceStatus status);
    void evaluateValue(float value);
    ControllerOperationResult applyPendingDecision();

    ThresholdControllerConfiguration configuration_;
    ModuleActuatorReference moduleTarget_;
    IMeasurementResolver& measurementResolver_;
    IOnOffActuatorResolver& actuatorResolver_;
    IMonotonicClock& monotonicClock_;
    ILogger& logger_;
    ControllerId controllerId_;
    String controllerName_;
    bool running_ = false;
    bool sourceAvailable_ = false;
    bool sourceAvailabilityKnown_ = false;
    bool hasLatestSnapshot_ = false;
    bool latestMeasurementValid_ = false;
    bool latestNumericValueAvailable_ = false;
    float latestNumericValue_ = 0.0F;
    bool latestSnapshotStale_ = false;
    uint32_t latestSnapshotAgeMs_ = 0;
    bool hasProcessedRevision_ = false;
    uint32_t lastProcessedRevision_ = 0;
    ThresholdDecision decision_ = ThresholdDecision::Unknown;
    ThresholdReason reason_ = ThresholdReason::NotStarted;
    bool targetAvailable_ = false;
    bool outputApplicationPending_ = false;
    bool targetUnavailabilityLogged_ = false;
    bool targetOperationFailureLogged_ = false;
    SourceStatus sourceStatus_ = SourceStatus::Unknown;
};

} // namespace EnvNode
