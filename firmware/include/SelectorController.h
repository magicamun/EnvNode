#pragma once
#include "ControllerSlotConfiguration.h"
#include "IController.h"
#include "IEnumValueReader.h"
#include "IThresholdReasonProvider.h"
#include "IOnOffActuatorResolver.h"

namespace EnvNode {
class SelectorController : public IController {
public:
    SelectorController(const SelectorControllerConfiguration& configuration,
        const ModuleActuatorReference& moduleTarget, const IEnumValueReader& values,
        IOnOffActuatorResolver& actuators)
        : configuration_(configuration), moduleTarget_(moduleTarget), values_(values), actuators_(actuators) {}
    // Bound once to the decision source in the same staged runtime composition.
    void bindSource(const IThresholdReasonProvider* source) { source_ = source; }
    ControllerOperationResult begin() override;
    ControllerOperationResult service() override;
    ControllerOperationResult stop() override;
    bool running() const { return running_; }
    bool targetAvailable() const { return targetAvailable_; }
    bool outputApplicationPending() const { return pending_; }
    ThresholdDecision decision() const { return decision_; }
private:
    SelectorControllerConfiguration configuration_;
    ModuleActuatorReference moduleTarget_;
    const IEnumValueReader& values_;
    IOnOffActuatorResolver& actuators_;
    const IThresholdReasonProvider* source_ = nullptr;
    bool running_ = false;
    bool targetAvailable_ = false;
    bool pending_ = false;
    ThresholdDecision decision_ = ThresholdDecision::Unknown;
};
}
