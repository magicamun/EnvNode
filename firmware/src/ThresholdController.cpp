#include "ThresholdController.h"

#include <climits>
#include <cmath>

namespace EnvNode {
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
    IMeasurementResolver& measurementResolver,
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger)
    : configuration_(configuration)
    , measurementResolver_(measurementResolver)
    , actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger) {
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
    float value = 0.0F;
    if (found) {
        latestMeasurementValid_ = snapshot.measurement.valid;
        latestSnapshotAgeMs_ = monotonicClock_.nowMs() - snapshot.acceptedMonotonicMs;
        latestSnapshotStale_ = latestSnapshotAgeMs_ > configuration_.maxMeasurementAgeMs;
        const MeasurementTypeMetadata& metadata =
            measurementTypeMetadata(snapshot.measurement.type);
        latestNumericValueAvailable_ = snapshot.measurement.value.tryGetFloatingPoint(value);
        if (latestNumericValueAvailable_) latestNumericValue_ = value;
        usable = snapshot.measurement.source == configuration_.source.sensorId
            && snapshot.measurement.type == configuration_.source.measurementType
            && snapshot.measurement.valid
            && latestNumericValueAvailable_
            && std::isfinite(value)
            && metadata.expectedValueKind == ValueKind::FloatingPoint
            && metadata.semantics == MeasurementSemantics::State
            && !latestSnapshotStale_;
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
    updateSourceAvailability(usable);

    return outputApplicationPending_
        ? applyPendingDecision() : ControllerOperationResult::NoAction;
}

ControllerOperationResult ThresholdController::stop() {
    running_ = false;
    sourceAvailable_ = false;
    decision_ = ThresholdDecision::Unknown;
    outputApplicationPending_ = false;
    IOnOffActuator* actuator =
        actuatorResolver_.onOffActuator(configuration_.targetActuatorId);
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
        && isValidActuatorId(configuration_.targetActuatorId)
        && std::isfinite(configuration_.onThreshold)
        && std::isfinite(configuration_.offThreshold)
        && configuration_.offThreshold < configuration_.onThreshold
        && configuration_.maxMeasurementAgeMs > 0
        && configuration_.maxMeasurementAgeMs <= INT32_MAX;
}

void ThresholdController::updateSourceAvailability(bool available) {
    if (sourceAvailabilityKnown_ && sourceAvailable_ != available) {
        logger_.printf("Threshold source Sensor %u %s\n",
            static_cast<unsigned int>(configuration_.source.sensorId),
            available ? "available" : "unavailable");
    }
    sourceAvailable_ = available;
    sourceAvailabilityKnown_ = true;
}

void ThresholdController::evaluateValue(float value) {
    ThresholdDecision next = decision_;
    if (value >= configuration_.onThreshold) {
        next = ThresholdDecision::On;
    } else if (value <= configuration_.offThreshold) {
        next = ThresholdDecision::Off;
    }
    if (next == decision_ || next == ThresholdDecision::Unknown) return;
    logger_.printf("Threshold decision %s -> %s\n",
        decisionName(decision_), decisionName(next));
    decision_ = next;
    outputApplicationPending_ = true;
}

ControllerOperationResult ThresholdController::applyPendingDecision() {
    IOnOffActuator* actuator =
        actuatorResolver_.onOffActuator(configuration_.targetActuatorId);
    if (actuator == nullptr) {
        targetAvailable_ = false;
        if (!targetUnavailabilityLogged_) {
            logger_.printf("Threshold target actuator %u unavailable\n",
                static_cast<unsigned int>(configuration_.targetActuatorId));
            targetUnavailabilityLogged_ = true;
        }
        return ControllerOperationResult::TargetUnavailable;
    }
    targetAvailable_ = true;
    if (targetUnavailabilityLogged_) {
        logger_.printf("Threshold target actuator %u available again\n",
            static_cast<unsigned int>(configuration_.targetActuatorId));
        targetUnavailabilityLogged_ = false;
    }
    const OnOffState desired = decision_ == ThresholdDecision::On
        ? OnOffState::On : OnOffState::Off;
    if (actuator->setState(desired) != ActuatorOperationResult::Completed) {
        return ControllerOperationResult::ActuatorOperationFailed;
    }
    outputApplicationPending_ = false;
    return ControllerOperationResult::Completed;
}

} // namespace EnvNode
