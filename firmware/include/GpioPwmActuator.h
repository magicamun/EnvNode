#pragma once

#include "HardwareResources.h"
#include "ILevelActuator.h"
#include "Logger.h"

namespace EnvNode {

constexpr uint32_t GpioPwmFrequencyHz = 5000;
constexpr uint8_t GpioPwmResolutionBits = 8;
constexpr uint32_t GpioPwmMaximumDuty = (1U << GpioPwmResolutionBits) - 1U;

uint32_t gpioPwmDutyForLevel(ActuatorLevel level);

class GpioPwmActuator : public ILevelActuator {
public:
    GpioPwmActuator(
        const HardwareResourceAssignment& hardware,
        uint8_t channel,
        ILogger& logger);

    ActuatorOperationResult begin() override;
    ActuatorOperationResult shutdown() override;
    ActuatorOperationResult setState(OnOffState state) override;
    OnOffState state() const override;
    ActuatorOperationResult setLevel(ActuatorLevel level) override;
    ActuatorLevel level() const override;
    bool initialized() const override;

private:
    HardwareResourceAssignment hardware_;
    uint8_t channel_;
    ILogger& logger_;
    ActuatorLevel level_ = ActuatorLevel::off();
    bool initialized_ = false;
};

} // namespace EnvNode
