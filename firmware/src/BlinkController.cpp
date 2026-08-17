#include "BlinkController.h"

#include <climits>

namespace EnvNode {

BlinkController::BlinkController(
    const BlinkControllerConfiguration& configuration,
    IOnOffActuatorResolver& actuatorResolver,
    IMonotonicClock& monotonicClock,
    ILogger& logger)
    : configuration_(configuration)
    , actuatorResolver_(actuatorResolver)
    , monotonicClock_(monotonicClock)
    , logger_(logger) {
}

ControllerOperationResult BlinkController::begin() {
    if (!isValidActuatorId(configuration_.targetActuatorId)
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
        actuatorResolver_.onOffActuator(configuration_.targetActuatorId);
    if (actuator == nullptr) {
        targetAvailable_ = false;
        phase_ = BlinkPhase::WaitingForTarget;
        if (!unavailabilityLogged_) {
            logger_.printf("Controller target actuator %u unavailable\n",
                static_cast<unsigned int>(configuration_.targetActuatorId));
            unavailabilityLogged_ = true;
        }
        return ControllerOperationResult::TargetUnavailable;
    }
    targetAvailable_ = true;
    if (unavailabilityLogged_) {
        logger_.printf("Controller target actuator %u available again\n",
            static_cast<unsigned int>(configuration_.targetActuatorId));
        unavailabilityLogged_ = false;
    }
    if (actuator->setState(state) != ActuatorOperationResult::Completed) {
        phase_ = BlinkPhase::WaitingForTarget;
        return ControllerOperationResult::ActuatorOperationFailed;
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
