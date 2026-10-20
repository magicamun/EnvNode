// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <RainDetectorPolicy.h>
#include "IController.h"
#include "IRainFrequencyResolver.h"
#include "IOnOffActuatorResolver.h"
#include "IMonotonicClock.h"
#include "Logger.h"
namespace EnvNode {
class RainDetectorController : public IController {
public:
    RainDetectorController(const RainDetectorControllerConfiguration& config,
        const ModuleActuatorReference& module, IRainFrequencyResolver& source,
        IOnOffActuatorResolver& actors, IMonotonicClock& clock, ILogger& logger)
        : config_(config), module_(module), source_(source), actors_(actors), clock_(clock), logger_(logger) {}
    ControllerOperationResult begin() override;
    ControllerOperationResult stop() override;
    ControllerOperationResult service() override;
    uint32_t wettingCount() const { return wettingCount_; }
    bool running() const { return running_; }
    bool targetAvailable() const { return target()!=nullptr; }
    const RainDetectorPolicyResult& result() const { return policy_.result(); }
private:
    IOnOffActuator* target() const;
    ControllerOperationResult apply(bool heating);
    RainDetectorControllerConfiguration config_;
    ModuleActuatorReference module_;
    IRainFrequencyResolver& source_;
    IOnOffActuatorResolver& actors_;
    IMonotonicClock& clock_;
    ILogger& logger_;
    RainDetectorPolicy policy_;
    bool running_ = false, processed_ = false, applied_ = false;
    uint32_t revision_ = 0, wettingCount_ = 0;
    uint64_t endUs_ = 0;
    OnOffState lastOutput_ = OnOffState::Off;
};
} // namespace EnvNode
