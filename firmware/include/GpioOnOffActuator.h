#pragma once

#include "HardwareResources.h"
#include "IOnOffActuator.h"
#include "Logger.h"

namespace EnvNode {

class GpioOnOffActuator : public IOnOffActuator {
public:
    GpioOnOffActuator(
        const HardwareResourceAssignment& hardware,
        ILogger& logger);

    ActuatorOperationResult begin() override;
    ActuatorOperationResult setState(OnOffState state) override;
    OnOffState state() const override;
    bool initialized() const override;

private:
    HardwareResourceAssignment hardware_;
    ILogger& logger_;
    OnOffState state_ = OnOffState::Off;
    bool initialized_ = false;
};

} // namespace EnvNode
