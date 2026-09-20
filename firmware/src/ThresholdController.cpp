#include "ThresholdController.h"

#include <climits>
#include <cmath>
#include "UnitConverter.h"

namespace EnvNode {
namespace {
IOnOffActuator* resolveTarget(
    IOnOffActuatorResolver& resolver,
    const ThresholdControllerConfiguration& configuration,
    const ModuleActuatorReference& moduleTarget) {
    return validModuleActuatorReference(moduleTarget)
        ? resolver.onOffActuator(moduleTarget)
        : resolver.onOffActuator(configuration.targetActuatorId);
}
}
namespace {

const char* decisionName(ThresholdDecision decision) {
    switch (decision) {
        case ThresholdDecision::On: return "On";
        case ThresholdDecision::Off: return "Off";
        case ThresholdDecision::Unknown:
        default: return "Unknown";
    }
}

} // namespace

ThresholdController::ThresholdController(
    const ThresholdControllerConfiguration& configuration,
    const ModuleActuatorReference& moduleTarget,
    IMeasurementResolver& measurementResolver,
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger,
    ControllerId controllerId,
    const String& controllerName)
    : configuration_(configuration)
    , moduleTarget_(moduleTarget)
    , measurementResolver_(measurementResolver)
    , actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger)
    , controllerId_(controllerId)
    , controllerName_(controllerName) {
}

ControllerOperationResult ThresholdController::begin() {
    if (!configurationValid()) return ControllerOperationResult::InvalidConfiguration;
    running_ = true;
    sourceAvailable_ = false;
    sourceAvailabilityKnown_ = false;
    hasLatestSnapshot_ = false;
    latestMeasurementValid_ = false;
    latestNumericValueAvailable_ = false;
    latestNumericValue_ = 0.0F;
    latestSnapshotStale_ = false;
    latestSnapshotAgeMs_ = 0;
    hasProcessedRevision_ = false;
    lastProcessedRevision_ = 0;
    decision_ = ThresholdDecision::Unknown;
    targetAvailable_ = false;
    outputApplicationPending_ = false;
    targetUnavailabilityLogged_ = false;
    targetOperationFailureLogged_ = false;
    sourceStatus_ = SourceStatus::Unknown;
    const ControllerOperationResult result = service();
    return result == ControllerOperationResult::NoAction
        ? ControllerOperationResult::Completed : result;
}

ControllerOperationResult ThresholdController::service() {
    if (!running_) return ControllerOperationResult::NotRunning;

    MeasurementSnapshot snapshot;
    const bool found = measurementResolver_.latest(configuration_.source, snapshot);
    hasLatestSnapshot_ = found;
    bool usable = false;
    SourceStatus sourceStatus = SourceStatus::Unavailable;
    float value = 0.0F;
    if (found) {
        latestMeasurementValid_ = snapshot.measurement.valid;
        latestSnapshotAgeMs_ = monotonicClock_.nowMs() - snapshot.acceptedMonotonicMs;
        latestSnapshotStale_ = latestSnapshotAgeMs_ > configuration_.maxMeasurementAgeMs;
        const MeasurementTypeMetadata& metadata =
            measurementTypeMetadata(snapshot.measurement.type);
        latestNumericValueAvailable_ = snapshot.measurement.value.tryGetFloatingPoint(value);
        if (latestNumericValueAvailable_) latestNumericValue_ = value;
        const bool compatible = snapshot.measurement.source == configuration_.source.sensorId
            && snapshot.measurement.type == configuration_.source.measurementType
            && snapshot.measurement.valid
            && latestNumericValueAvailable_
            && std::isfinite(value)
            && metadata.expectedValueKind == ValueKind::FloatingPoint
            && metadata.semantics == MeasurementSemantics::State;
        usable = compatible && !latestSnapshotStale_;
        sourceStatus = compatible && latestSnapshotStale_
            ? SourceStatus::Stale
            : usable ? SourceStatus::Available : SourceStatus::Unavailable;
        if (!hasProcessedRevision_ || snapshot.revision != lastProcessedRevision_) {
            hasProcessedRevision_ = true;
            lastProcessedRevision_ = snapshot.revision;
            if (usable) evaluateValue(value);
        }
    } else {
        latestMeasurementValid_ = false;
        latestNumericValueAvailable_ = false;
        latestSnapshotStale_ = false;
        latestSnapshotAgeMs_ = 0;
    }
    updateSourceAvailability(sourceStatus);

    return outputApplicationPending_
        ? applyPendingDecision() : ControllerOperationResult::NoAction;
}

ControllerOperationResult ThresholdController::stop() {
    running_ = false;
    sourceAvailable_ = false;
    decision_ = ThresholdDecision::Unknown;
    outputApplicationPending_ = false;
    IOnOffActuator* actuator =
        resolveTarget(actuatorResolver_, configuration_, moduleTarget_);
    if (actuator == nullptr) {
        targetAvailable_ = false;
        return ControllerOperationResult::TargetUnavailable;
    }
    targetAvailable_ = true;
    return actuator->setState(OnOffState::Off) == ActuatorOperationResult::Completed
        ? ControllerOperationResult::Completed
        : ControllerOperationResult::ActuatorOperationFailed;
}

bool ThresholdController::running() const { return running_; }
bool ThresholdController::sourceAvailable() const { return sourceAvailable_; }
bool ThresholdController::hasLatestSnapshot() const { return hasLatestSnapshot_; }
bool ThresholdController::latestMeasurementValid() const { return latestMeasurementValid_; }
bool ThresholdController::latestNumericValueAvailable() const {
    return latestNumericValueAvailable_;
}
float ThresholdController::latestNumericValue() const { return latestNumericValue_; }
bool ThresholdController::latestSnapshotStale() const { return latestSnapshotStale_; }
uint32_t ThresholdController::latestSnapshotAgeMs() const { return latestSnapshotAgeMs_; }
ThresholdDecision ThresholdController::decision() const { return decision_; }
bool ThresholdController::targetAvailable() const { return targetAvailable_; }
bool ThresholdController::outputApplicationPending() const {
    return outputApplicationPending_;
}
bool ThresholdController::hasProcessedRevision() const { return hasProcessedRevision_; }
uint32_t ThresholdController::lastProcessedRevision() const {
    return lastProcessedRevision_;
}

bool ThresholdController::configurationValid() const {
    const MeasurementTypeMetadata& metadata =
        measurementTypeMetadata(configuration_.source.measurementType);
    return isValidSensorId(configuration_.source.sensorId)
        && metadata.expectedValueKind == ValueKind::FloatingPoint
        && metadata.semantics == MeasurementSemantics::State
        && (validModuleActuatorReference(moduleTarget_)
            || isValidActuatorId(configuration_.targetActuatorId))
        && std::isfinite(configuration_.onThreshold)
        && std::isfinite(configuration_.offThreshold)
        && validThresholdOrdering(configuration_.direction,
            configuration_.onThreshold, configuration_.offThreshold)
        && configuration_.maxMeasurementAgeMs > 0
        && configuration_.maxMeasurementAgeMs <= INT32_MAX;
}

void ThresholdController::updateSourceAvailability(SourceStatus status) {
    if (status != sourceStatus_) {
        const char* type = measurementTypeStableId(configuration_.source.measurementType);
        if (status == SourceStatus::Available) {
            if (sourceStatus_ != SourceStatus::Unknown) {
                logger_.infof(
                    "Controller %u \"%s\": Measurement source Sensor %u %s available",
                    static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                    static_cast<unsigned int>(configuration_.source.sensorId), type);
            }
        } else if (status == SourceStatus::Stale) {
            logger_.warnf(
                "Controller %u \"%s\": Measurement source Sensor %u %s stale, age=%lu ms, maximum=%lu ms",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.source.sensorId), type,
                static_cast<unsigned long>(latestSnapshotAgeMs_),
                static_cast<unsigned long>(configuration_.maxMeasurementAgeMs));
        } else if (status == SourceStatus::Unavailable) {
            logger_.warnf(
                "Controller %u \"%s\": Measurement source Sensor %u %s unavailable",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.source.sensorId), type);
        }
    }
    sourceStatus_ = status;
    sourceAvailable_ = status == SourceStatus::Available;
    sourceAvailabilityKnown_ = true;
}

void ThresholdController::evaluateValue(float value) {
    const ThresholdDecision previous = decision_;
    ThresholdDecision next = decision_;
    if (configuration_.direction == ThresholdDirection::OnAbove) {
        if (value >= configuration_.onThreshold) next = ThresholdDecision::On;
        else if (value <= configuration_.offThreshold) next = ThresholdDecision::Off;
    } else {
        if (value <= configuration_.onThreshold) next = ThresholdDecision::On;
        else if (value >= configuration_.offThreshold) next = ThresholdDecision::Off;
    }
    const MeasurementTypeMetadata& metadata =
        measurementTypeMetadata(configuration_.source.measurementType);
    const char* type = metadata.stableId;
    const char* unit = UnitConverter::symbol(metadata.canonicalUnit);
    const bool hasUnit = unit != nullptr && unit[0] != '\0';
    if (!hasUnit) unit = "";
    logger_.debugf(
        "Controller %u \"%s\": evaluate Sensor %u %s=%g%s%s, thresholds off=%g on=%g%s%s, age=%lu ms/%lu ms, decision=%s -> %s",
        static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
        static_cast<unsigned int>(configuration_.source.sensorId), type,
        static_cast<double>(value), hasUnit ? " " : "", unit,
        static_cast<double>(configuration_.offThreshold),
        static_cast<double>(configuration_.onThreshold),
        hasUnit ? " " : "", unit,
        static_cast<unsigned long>(latestSnapshotAgeMs_),
        static_cast<unsigned long>(configuration_.maxMeasurementAgeMs),
        decisionName(previous), decisionName(next));
    if (next == previous || next == ThresholdDecision::Unknown) return;
    const float triggeringThreshold = next == ThresholdDecision::On
        ? configuration_.onThreshold : configuration_.offThreshold;
    logger_.infof(
        "Controller %u \"%s\": Threshold decision %s -> %s, Sensor %u %s=%g%s%s, %s threshold=%g%s%s",
        static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
        decisionName(previous), decisionName(next),
        static_cast<unsigned int>(configuration_.source.sensorId), type,
        static_cast<double>(value), hasUnit ? " " : "", unit,
        next == ThresholdDecision::On ? "on" : "off",
        static_cast<double>(triggeringThreshold),
        hasUnit ? " " : "", unit);
    decision_ = next;
    outputApplicationPending_ = true;
}

ControllerOperationResult ThresholdController::applyPendingDecision() {
    IOnOffActuator* actuator =
        resolveTarget(actuatorResolver_, configuration_, moduleTarget_);
    if (actuator == nullptr) {
        targetAvailable_ = false;
        if (!targetUnavailabilityLogged_) {
            logger_.warnf("Controller %u \"%s\": target Actuator %u unavailable",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.targetActuatorId));
            targetUnavailabilityLogged_ = true;
        }
        return ControllerOperationResult::TargetUnavailable;
    }
    targetAvailable_ = true;
    if (targetUnavailabilityLogged_) {
        logger_.infof("Controller %u \"%s\": target Actuator %u available",
            static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
            static_cast<unsigned int>(configuration_.targetActuatorId));
        targetUnavailabilityLogged_ = false;
    }
    const OnOffState desired = decision_ == ThresholdDecision::On
        ? OnOffState::On : OnOffState::Off;
    const ActuatorOperationResult operationResult = actuator->setState(desired);
    if (operationResult != ActuatorOperationResult::Completed) {
        if (!targetOperationFailureLogged_) {
            logger_.warnf(
                "Controller %u \"%s\": target Actuator %u could not apply %s, result=%u; retry pending",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.targetActuatorId),
                desired == OnOffState::On ? "On" : "Off",
                static_cast<unsigned int>(operationResult));
            targetOperationFailureLogged_ = true;
        }
        return ControllerOperationResult::ActuatorOperationFailed;
    }
    if (targetOperationFailureLogged_) {
        logger_.infof(
            "Controller %u \"%s\": target Actuator %u applied %s after retry",
            static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
            static_cast<unsigned int>(configuration_.targetActuatorId),
            desired == OnOffState::On ? "On" : "Off");
        targetOperationFailureLogged_ = false;
    }
    outputApplicationPending_ = false;
    return ControllerOperationResult::Completed;
}

} // namespace EnvNode
