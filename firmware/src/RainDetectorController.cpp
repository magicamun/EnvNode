// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include "RainDetectorController.h"
namespace EnvNode {
IOnOffActuator* RainDetectorController::target() const {
    return validModuleActuatorReference(module_) ? actors_.onOffActuator(module_)
        : actors_.onOffActuator(config_.targetActuatorId);
}
ControllerOperationResult RainDetectorController::apply(bool heating) {
    auto* actor=target();
    if (!actor || !actor->initialized()) { applied_=false; return ControllerOperationResult::TargetUnavailable; }
    const auto desired=heating?OnOffState::On:OnOffState::Off;
    if (actor->outputPercent()==(heating?100:0) && applied_) return ControllerOperationResult::NoAction;
    if (actor->outputPercent()!=(heating?100:0) || !applied_) {
        if (actor->setState(desired)!=ActuatorOperationResult::Completed) {
            applied_=false; return ControllerOperationResult::ActuatorOperationFailed;
        }
        source_.rainFrequencyHeaterChanged(config_.sourceSensorId);
    }
    lastOutput_=desired; applied_=true;
    return ControllerOperationResult::Completed;
}
ControllerOperationResult RainDetectorController::begin() {
    policy_.disable(); processed_=false; applied_=false; wettingCount_=0;
    if (!config_.policy.valid() || !policy_.configure(config_.policy)) return ControllerOperationResult::InvalidConfiguration;
    policy_.enable(); running_=true;
    return apply(false);
}
ControllerOperationResult RainDetectorController::stop() {
    running_=false; policy_.disable(); processed_=false;
    return apply(false);
}
ControllerOperationResult RainDetectorController::service() {
    if (!running_) return ControllerOperationResult::NotRunning;
    auto* actor=target();
    if (!actor || !actor->initialized()) {
        policy_.update(clock_.nowMs(),NAN,0);
        applied_=false;
        return ControllerOperationResult::TargetUnavailable;
    }
    if (applied_ && actor->outputPercent()!=(lastOutput_==OnOffState::On?100:0)) {
        logger_.warn("RainDetector: manual actuator override disarms controller");
        return stop();
    }
    if (!applied_) {
        policy_.disable(); policy_.enable(); processed_=false;
        return apply(false); // Replacement/unavailable target resumes safely OFF.
    }
    RainFrequencySnapshot snapshot;
    const bool available=source_.rainFrequencySnapshot(config_.sourceSensorId,snapshot);
    const auto before=policy_.result();
    if (!available || static_cast<uint32_t>(clock_.nowMs()-snapshot.acceptedMs)>1500) {
        policy_.update(clock_.nowMs(),NAN,0);
    } else if (!processed_ || revision_!=snapshot.revision || endUs_!=snapshot.window.endUs) {
        revision_=snapshot.revision; endUs_=snapshot.window.endUs; processed_=true;
        policy_.update(snapshot.window.endUs/1000,
            snapshot.window.valid()?snapshot.window.frequencyHz:NAN,
            actor->outputPercent());
    } else return ControllerOperationResult::NoAction;
    const auto& result=policy_.result();
    if (result.wetting) {
        ++wettingCount_;
        lastWettingMs_ = clock_.nowMs();
    }
    if (result.rain == RainDetectorRainState::EndEstimated
        && before.rain != RainDetectorRainState::EndEstimated)
        rainEndConfirmationMs_ = clock_.nowMs();
    if (result.wetting || result.valid!=before.valid || result.rain!=before.rain || result.heating!=before.heating || result.dry!=before.dry)
        logger_.infof("RainDetector: rain=%s valid=%u wetting=%u heating=%u dry=%u",
            rainDetectorRainStateName(result.rain),result.valid,result.wetting,result.heating,result.dry);
    return apply(result.heating);
}
} // namespace EnvNode
