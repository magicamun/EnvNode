// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <RainDetectorPolicy.h>
#include "IController.h"
#include "IRainFrequencyResolver.h"
#include "IOnOffActuatorResolver.h"
#include "IMonotonicClock.h"
#include "Logger.h"
#include "ControllerId.h"
#include "IDiagnosticRecorder.h"
#include "IMeasurementResolver.h"
namespace EnvNode {
class RainDetectorController : public IController {
public:
    RainDetectorController(const RainDetectorControllerConfiguration& config,
        const ModuleActuatorReference& module, IRainFrequencyResolver& source,
        IOnOffActuatorResolver& actors, IMonotonicClock& clock, ILogger& logger,
        ControllerId id = InvalidControllerId, IDiagnosticRecorder& diagnostics = diagnosticRecorder(),
        const IMeasurementResolver* climate = nullptr)
        : config_(config), module_(module), source_(source), actors_(actors), clock_(clock), logger_(logger),
          id_(id), diagnostics_(diagnostics), climate_(climate) {}
    ControllerOperationResult begin() override;
    ControllerOperationResult stop() override;
    ControllerOperationResult service() override;
    uint32_t wettingCount() const { return wettingCount_; }
    bool rainStartKnown() const { return rainStartKnown_; }
    bool rainEndDetectionKnown() const { return rainEndDetectionKnown_; }
    uint32_t rainStartMs() const { return rainStartMs_; }
    uint32_t lastWettingMs() const { return lastWettingMs_; }
    uint32_t rainEndConfirmationMs() const { return rainEndConfirmationMs_; }
    uint32_t rainStartAgeMs() const { return clock_.nowMs() - rainStartMs_; }
    uint32_t lastWettingAgeMs() const { return clock_.nowMs() - lastWettingMs_; }
    uint32_t rainEndConfirmationAgeMs() const { return clock_.nowMs() - rainEndConfirmationMs_; }
    bool running() const { return running_; }
    bool targetAvailable() const { return target()!=nullptr; }
    const RainDetectorPolicyResult& result() const { return effectiveResult_; }
private:
    void updateDewDemand();
    ControllerOperationResult applyDemand(const char* reason);
    IOnOffActuator* target() const;
    ControllerOperationResult apply(bool heating);
    ControllerOperationResult stopWithReason(const char* reason);
    ControllerOperationResult report(ControllerOperationResult operation,const char* reason);
    void recordDiagnosticContext(DiagnosticSource source);
    void recordDiagnosticDecision(DiagnosticSource source);
    void recordDiagnosticOutput(DiagnosticSource source,ControllerOperationResult operation,const char* reason);
    RainDetectorControllerConfiguration config_;
    ModuleActuatorReference module_;
    IRainFrequencyResolver& source_;
    IOnOffActuatorResolver& actors_;
    IMonotonicClock& clock_;
    ILogger& logger_;
    ControllerId id_;
    IDiagnosticRecorder& diagnostics_;
    const IMeasurementResolver* climate_;
    RainDetectorPolicyResult effectiveResult_;
    bool dewDemand_ = false, climateValid_ = false;
    float dewGapC_ = NAN;
    bool dewDiagnosticKnown_ = false;
    uint8_t dewDiagnosticFlags_ = 0;
    RainDetectorPolicy policy_;
    bool running_ = false, processed_ = false, applied_ = false;
    uint32_t revision_ = 0, wettingCount_ = 0;
    uint32_t lastWettingMs_ = 0, rainEndConfirmationMs_ = 0, rainStartMs_ = 0;
    bool rainStartKnown_ = false, rainEndDetectionKnown_ = false;
    uint64_t endUs_ = 0;
    OnOffState lastOutput_ = OnOffState::Off;
    bool diagnosticsActive_ = false,diagnosticDecisionKnown_ = false,diagnosticOutputKnown_ = false;
    uint8_t diagnosticDecisionFlags_ = 0;
    uint16_t diagnosticOutputFlags_ = 0;
    uint32_t diagnosticContextAtMs_ = 0;
    const char* diagnosticReason_ = nullptr;
};
} // namespace EnvNode
