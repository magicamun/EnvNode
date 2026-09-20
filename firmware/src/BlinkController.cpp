#include "BlinkController.h"

#include <climits>

namespace EnvNode {
namespace {
IOnOffActuator* resolveTarget(
    IOnOffActuatorResolver& resolver,
    const BlinkControllerConfiguration& configuration,
    const ModuleActuatorReference& moduleTarget) {
    return validModuleActuatorReference(moduleTarget)
        ? resolver.onOffActuator(moduleTarget)
        : resolver.onOffActuator(configuration.targetActuatorId);
}
}

BlinkController::BlinkController(
    const BlinkControllerConfiguration& configuration,
    const ModuleActuatorReference& moduleTarget,
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger,
    ControllerId controllerId,
    const String& controllerName)
    : configuration_(configuration)
    , moduleTarget_(moduleTarget)
    , actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger)
    , controllerId_(controllerId)
    , controllerName_(controllerName) {
}

ControllerOperationResult BlinkController::begin() {
    if ((!validModuleActuatorReference(moduleTarget_)
            && !isValidActuatorId(configuration_.targetActuatorId))
        || configuration_.onDurationMs == 0
        || configuration_.onDurationMs > INT32_MAX
        || configuration_.offDurationMs == 0
        || configuration_.offDurationMs > INT32_MAX) {
        return ControllerOperationResult::InvalidConfiguration;
    }
    running_ = true;
    phase_ = BlinkPhase::WaitingForTarget;
    return startCycle();
}

ControllerOperationResult BlinkController::service() {
    if (!running_) return ControllerOperationResult::NotRunning;
    if (phase_ == BlinkPhase::WaitingForTarget) return startCycle();

    const uint32_t nowMs = monotonicClock_.nowMs();
    if (!deadlineReached(nowMs, nextTransitionMs_)) {
        return ControllerOperationResult::NoAction;
    }
    if (phase_ == BlinkPhase::On) {
        return command(OnOffState::Off, BlinkPhase::Off);
    }
    if (phase_ == BlinkPhase::Off) {
        return command(OnOffState::On, BlinkPhase::On);
    }
    return ControllerOperationResult::NotRunning;
}

ControllerOperationResult BlinkController::stop() {
    running_ = false;
    phase_ = BlinkPhase::Stopped;
    nextTransitionMs_ = 0;
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

BlinkPhase BlinkController::phase() const { return phase_; }
bool BlinkController::running() const { return running_; }
bool BlinkController::targetAvailable() const { return targetAvailable_; }
uint32_t BlinkController::nextTransitionMs() const { return nextTransitionMs_; }

bool BlinkController::deadlineReached(uint32_t nowMs, uint32_t deadlineMs) {
    return static_cast<int32_t>(nowMs - deadlineMs) >= 0;
}

ControllerOperationResult BlinkController::command(
    OnOffState state,
    BlinkPhase successfulPhase) {
    IOnOffActuator* actuator =
        resolveTarget(actuatorResolver_, configuration_, moduleTarget_);
    if (actuator == nullptr) {
        targetAvailable_ = false;
        phase_ = BlinkPhase::WaitingForTarget;
        if (!unavailabilityLogged_) {
            logger_.warnf("Controller %u \"%s\": target Actuator %u unavailable",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.targetActuatorId));
            unavailabilityLogged_ = true;
        }
        return ControllerOperationResult::TargetUnavailable;
    }
    targetAvailable_ = true;
    if (unavailabilityLogged_) {
        logger_.infof("Controller %u \"%s\": target Actuator %u available",
            static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
            static_cast<unsigned int>(configuration_.targetActuatorId));
        unavailabilityLogged_ = false;
    }
    const ActuatorOperationResult operationResult = actuator->setState(state);
    if (operationResult != ActuatorOperationResult::Completed) {
        phase_ = BlinkPhase::WaitingForTarget;
        if (!operationFailureLogged_) {
            logger_.warnf(
                "Controller %u \"%s\": target Actuator %u could not apply %s, result=%u; retry pending",
                static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
                static_cast<unsigned int>(configuration_.targetActuatorId),
                state == OnOffState::On ? "On" : "Off",
                static_cast<unsigned int>(operationResult));
            operationFailureLogged_ = true;
        }
        return ControllerOperationResult::ActuatorOperationFailed;
    }
    if (operationFailureLogged_) {
        logger_.infof(
            "Controller %u \"%s\": target Actuator %u applied %s after retry",
            static_cast<unsigned int>(controllerId_), controllerName_.c_str(),
            static_cast<unsigned int>(configuration_.targetActuatorId),
            state == OnOffState::On ? "On" : "Off");
        operationFailureLogged_ = false;
    }
    phase_ = successfulPhase;
    const uint32_t duration = successfulPhase == BlinkPhase::On
        ? configuration_.onDurationMs : configuration_.offDurationMs;
    nextTransitionMs_ = monotonicClock_.nowMs() + duration;
    return ControllerOperationResult::Completed;
}

ControllerOperationResult BlinkController::startCycle() {
    return command(OnOffState::On, BlinkPhase::On);
}

} // namespace EnvNode
