#pragma once

#include "ControllerSlotConfiguration.h"
#include "IController.h"
#include "IMonotonicClock.h"
#include "IOnOffActuatorResolver.h"
#include "Logger.h"

namespace EnvNode {

enum class BlinkPhase : uint8_t {
    Stopped,
    WaitingForTarget,
    On,
    Off,
};

class BlinkController : public IController {
public:
    BlinkController(
        const BlinkControllerConfiguration& configuration,
        IOnOffActuatorResolver& actuatorResolver,
        IMonotonicClock& monotonicClock,
        ILogger& logger);

    ControllerOperationResult begin() override;
    ControllerOperationResult service() override;
    ControllerOperationResult stop() override;

    BlinkPhase phase() const;
    bool running() const;
    bool targetAvailable() const;
    uint32_t nextTransitionMs() const;

private:
    static bool deadlineReached(uint32_t nowMs, uint32_t deadlineMs);
    ControllerOperationResult command(OnOffState state, BlinkPhase successfulPhase);
    ControllerOperationResult startCycle();

    BlinkControllerConfiguration configuration_;
    IOnOffActuatorResolver& actuatorResolver_;
    IMonotonicClock& monotonicClock_;
    ILogger& logger_;
    BlinkPhase phase_ = BlinkPhase::Stopped;
    uint32_t nextTransitionMs_ = 0;
    bool running_ = false;
    bool targetAvailable_ = false;
    bool unavailabilityLogged_ = false;
};

} // namespace EnvNode
