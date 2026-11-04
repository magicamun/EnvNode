// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include "RainDetectorController.h"
#include <cstring>
namespace EnvNode {
namespace {
const char* operationName(ControllerOperationResult operation) {
    switch (operation) {
        case ControllerOperationResult::Completed: return "completed";
        case ControllerOperationResult::NoAction: return "no_action";
        case ControllerOperationResult::TargetUnavailable: return "target_unavailable";
        case ControllerOperationResult::ActuatorOperationFailed: return "actuator_operation_failed";
        case ControllerOperationResult::InvalidConfiguration: return "invalid_configuration";
        case ControllerOperationResult::NotRunning: return "not_running";
        default: return "controller_not_found";
    }
}
}
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
    rainStartKnown_ = rainEndDetectionKnown_ = false;
    if (!config_.policy.valid() || !policy_.configure(config_.policy))
        return report(ControllerOperationResult::InvalidConfiguration,"invalid_configuration");
    policy_.enable(); running_=true;
    return report(apply(false),"start");
}
ControllerOperationResult RainDetectorController::stop() {
    return stopWithReason("stop");
}
ControllerOperationResult RainDetectorController::stopWithReason(const char* reason) {
    running_=false; policy_.disable(); processed_=false;
    return report(apply(false),reason);
}
ControllerOperationResult RainDetectorController::service() {
    if (!running_) return report(ControllerOperationResult::NotRunning,"not_running");
    auto* actor=target();
    if (!actor || !actor->initialized()) {
        policy_.update(clock_.nowMs(),NAN,0);
        applied_=false;
        return report(ControllerOperationResult::TargetUnavailable,"target_unavailable");
    }
    if (applied_ && actor->outputPercent()!=(lastOutput_==OnOffState::On?100:0)) {
        logger_.warn("RainDetector: manual actuator override disarms controller");
        return stopWithReason("manual_override");
    }
    if (!applied_) {
        const bool startupPending = policy_.startupPending();
        policy_.disable(); policy_.enable(startupPending); processed_=false;
        return report(apply(false),"target_recovered"); // Replacement/unavailable target resumes safely OFF.
    }
    RainFrequencySnapshot snapshot;
    const bool available=source_.rainFrequencySnapshot(config_.sourceSensorId,snapshot);
    const auto before=policy_.result();
    const bool stale=available && static_cast<uint32_t>(clock_.nowMs()-snapshot.acceptedMs)>1500;
    const char* reason=!available?"no_input":stale?"stale_input":
        snapshot.window.valid()?"evaluation":"input_fault";
    if (!available || stale) {
        policy_.update(clock_.nowMs(),NAN,0);
    } else if (!processed_ || revision_!=snapshot.revision || endUs_!=snapshot.window.endUs) {
        revision_=snapshot.revision; endUs_=snapshot.window.endUs; processed_=true;
        policy_.update(snapshot.window.endUs/1000,
            snapshot.window.valid()?snapshot.window.frequencyHz:NAN,
            actor->outputPercent());
    } else return report(ControllerOperationResult::NoAction,reason);
    const auto& result=policy_.result();
    if (result.heating && !before.heating && !result.wetting)
        logger_.info("RainDetector: startup frequency below dry threshold; full drying heat requested");
    if (result.wetting) {
        if (before.rain != RainDetectorRainState::Active) {
            rainStartMs_ = clock_.nowMs();
            rainStartKnown_ = true;
        }
        ++wettingCount_;
        lastWettingMs_ = clock_.nowMs();
    }
    if (result.rain == RainDetectorRainState::EndEstimated
        && before.rain != RainDetectorRainState::EndEstimated) {
        rainEndConfirmationMs_ = clock_.nowMs();
        rainEndDetectionKnown_ = true;
    }
    if (result.wetting || result.valid!=before.valid || result.rain!=before.rain || result.heating!=before.heating || result.dry!=before.dry)
        logger_.infof("RainDetector: rain=%s valid=%u wetting=%u heating=%u dry=%u",
            rainDetectorRainStateName(result.rain),result.valid,result.wetting,result.heating,result.dry);
    return report(apply(result.heating),reason);
}

ControllerOperationResult RainDetectorController::report(ControllerOperationResult operation,const char* reason) {
    const DiagnosticSource source(DiagnosticSourceKind::Controller,id_);
    if (!diagnostics_.enabled(source)) {
        diagnosticsActive_=false;
        return operation;
    }
    const uint32_t now=clock_.nowMs();
    if (!diagnosticsActive_ || static_cast<uint32_t>(now-diagnosticContextAtMs_)>=60000) {
        diagnosticsActive_=true;diagnosticContextAtMs_=now;
        diagnosticDecisionKnown_=diagnosticOutputKnown_=false;
        recordDiagnosticContext(source);
    }
    recordDiagnosticDecision(source);
    recordDiagnosticOutput(source,operation,reason);
    return operation;
}

void RainDetectorController::recordDiagnosticContext(DiagnosticSource source) {
    {
        DiagnosticEvent event("rain_config");
        event.integer("sensor_id",config_.sourceSensorId);
        event.integer("target_actuator_id",config_.targetActuatorId);
        event.integer("dry_hz",config_.policy.dryHz);
        event.integer("hysteresis_hz",config_.policy.hysteresisHz);
        event.integer("dry_seconds",config_.policy.drySeconds);
        event.integer("end_seconds",config_.policy.endSeconds);
        (void)diagnostics_.record(source,event);
    }
    {
        DiagnosticEvent event("rain_edge_config");
        event.integer("sensor_id",config_.sourceSensorId);
        event.integer("edge_ms",config_.policy.edgeMs);
        event.integer("drop_hz",config_.policy.dropHz);
        event.integer("drop_permille",config_.policy.dropPermille);
        (void)diagnostics_.record(source,event);
    }
}

void RainDetectorController::recordDiagnosticDecision(DiagnosticSource source) {
    const auto& result=policy_.result();
    const uint8_t flags=(result.valid?1:0)|(result.wetting?2:0)|
        (static_cast<uint8_t>(result.rain)<<2)|(result.heating?16:0)|(result.dry?32:0);
    if (diagnosticDecisionKnown_ && diagnosticDecisionFlags_==flags) return;
    diagnosticDecisionKnown_=true;diagnosticDecisionFlags_=flags;
    DiagnosticEvent event("rain_decision");
    event.integer("sensor_id",config_.sourceSensorId);
    event.integer("revision",revision_);
    event.boolean("valid",result.valid);
    event.boolean("wetting",result.wetting);
    event.text("rain",rainDetectorRainStateName(result.rain));
    event.boolean("heating",result.heating);
    (void)diagnostics_.record(source,event);
}

void RainDetectorController::recordDiagnosticOutput(DiagnosticSource source,
    ControllerOperationResult operation,const char* reason) {
    auto* actor=target();
    const bool available=actor && actor->initialized();
    const uint8_t output=available?actor->outputPercent():0;
    // Completed and NoAction with the same output are one successful state.
    const auto stateOperation=operation==ControllerOperationResult::NoAction?
        ControllerOperationResult::Completed:operation;
    const uint16_t flags=(running_?1:0)|(policy_.result().dry?2:0)|
        (policy_.result().heating?4:0)|(available?8:0)|
        (static_cast<uint16_t>(output)<<4)|(static_cast<uint16_t>(stateOperation)<<11);
    if (diagnosticOutputKnown_ && diagnosticOutputFlags_==flags && !strcmp(diagnosticReason_,reason)) return;
    diagnosticOutputKnown_=true;diagnosticOutputFlags_=flags;diagnosticReason_=reason;
    DiagnosticEvent event("rain_output");
    event.integer("sensor_id",config_.sourceSensorId);
    event.boolean("running",running_);
    event.boolean("dry",policy_.result().dry);
    if (available) event.integer("output_percent",output);
    event.text("reason",reason);
    event.text("operation",operationName(operation));
    (void)diagnostics_.record(source,event);
}
} // namespace EnvNode
