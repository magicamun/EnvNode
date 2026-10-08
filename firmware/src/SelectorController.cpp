#include "SelectorController.h"
namespace EnvNode {
ControllerOperationResult SelectorController::begin() {
    const auto* definition = values_.valueDefinition(configuration_.modeValueId);
    if (!source_ || !definition || !validSelectorMapping(configuration_, *definition)
        || (!validModuleActuatorReference(moduleTarget_) && !isValidActuatorId(configuration_.targetActuatorId)))
        return ControllerOperationResult::InvalidConfiguration;
    running_ = true;
    pending_ = false;
    decision_ = ThresholdDecision::Unknown;
    return service();
}
ControllerOperationResult SelectorController::service() {
    if (!running_) return ControllerOperationResult::NotRunning;
    decision_ = ThresholdDecision::Unknown;
    pending_ = false;
    String mode;
    if (values_.valueCode(configuration_.modeValueId, mode)) {
        if (mode == configuration_.onCode) decision_ = ThresholdDecision::On;
        else if (mode == configuration_.offCode) decision_ = ThresholdDecision::Off;
        else if (mode == configuration_.automaticCode && source_ && source_->decisionCurrent())
            decision_ = source_->decision();
    }
    IOnOffActuator* target = validModuleActuatorReference(moduleTarget_)
        ? actuators_.onOffActuator(moduleTarget_) : actuators_.onOffActuator(configuration_.targetActuatorId);
    targetAvailable_ = target != nullptr && target->initialized();
    // No write and no retry of an earlier command while the desired state is unknown.
    if (decision_ != ThresholdDecision::On && decision_ != ThresholdDecision::Off)
        return ControllerOperationResult::NoAction;
    pending_ = true;
    if (!targetAvailable_) return ControllerOperationResult::TargetUnavailable;
    const OnOffState desired = decision_ == ThresholdDecision::On ? OnOffState::On : OnOffState::Off;
    if (target->state() == desired) { pending_ = false; return ControllerOperationResult::NoAction; }
    if (target->setState(desired) != ActuatorOperationResult::Completed)
        return ControllerOperationResult::ActuatorOperationFailed;
    pending_ = false;
    return ControllerOperationResult::Completed;
}
ControllerOperationResult SelectorController::stop() {
    running_ = false;
    pending_ = false;
    decision_ = ThresholdDecision::Unknown;
    return ControllerOperationResult::Completed;
}
}
