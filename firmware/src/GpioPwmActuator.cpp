#include "GpioPwmActuator.h"

#include <Arduino.h>
#include "ActuatorDiagnostics.h"

namespace EnvNode {

uint32_t gpioPwmDutyForLevel(ActuatorLevel level) {
    return (static_cast<uint32_t>(level.percent()) * GpioPwmMaximumDuty + 50U) / 100U;
}

GpioPwmActuator::GpioPwmActuator(
    const HardwareResourceAssignment& hardware,
    uint8_t channel,
    ILogger& logger, ActuatorId diagnosticId)
    : hardware_(hardware)
    , channel_(channel)
    , logger_(logger)
    , diagnosticId_(diagnosticId) {
}

ActuatorOperationResult GpioPwmActuator::begin() {
    const uint8_t previous = outputPercent();
    const HardwareResourceValidationResult validation = BoardCapabilities::current().validate(
        HardwareInterfaceKind::GPIO,
        hardware_,
        GpioCapability::DigitalOutput);
    if (validation != HardwareResourceValidationResult::Valid) {
        initialized_ = false;
        logger_.errorf(
            "GPIO PWM actuator initialization failed: invalid hardware resource (%u)",
            static_cast<unsigned int>(validation));
        recordActuatorOutput(diagnosticId_, hardware_.gpio.number, outputPercent(), 0, outputPercent(),
            ActuatorOperationResult::InvalidHardwareResource, "begin");
        return ActuatorOperationResult::InvalidHardwareResource;
    }
    if (ledcSetup(channel_, GpioPwmFrequencyHz, GpioPwmResolutionBits) == 0) {
        initialized_ = false;
        logger_.errorf("GPIO PWM actuator initialization failed: LEDC channel %u",
            static_cast<unsigned int>(channel_));
        recordActuatorOutput(diagnosticId_, hardware_.gpio.number, outputPercent(), 0, outputPercent(),
            ActuatorOperationResult::InvalidHardwareResource, "begin");
        return ActuatorOperationResult::InvalidHardwareResource;
    }

    ledcWrite(channel_, 0);
    ledcAttachPin(hardware_.gpio.number, channel_);
    level_ = ActuatorLevel::off();
    initialized_ = true;
    recordActuatorOutput(diagnosticId_, hardware_.gpio.number, previous, 0, 0,
        ActuatorOperationResult::Completed, "begin");
    logger_.debugf("GPIO PWM actuator initialized Off on GPIO%u channel=%u",
        hardware_.gpio.number, static_cast<unsigned int>(channel_));
    return ActuatorOperationResult::Completed;
}

ActuatorOperationResult GpioPwmActuator::setState(OnOffState state) {
    return setLevel(state == OnOffState::On
        ? ActuatorLevel::full() : ActuatorLevel::off());
}

OnOffState GpioPwmActuator::state() const {
    return level_.percent() == 0 ? OnOffState::Off : OnOffState::On;
}

ActuatorOperationResult GpioPwmActuator::setLevel(ActuatorLevel level) {
    if (!initialized_) {
        recordActuatorOutput(diagnosticId_, hardware_.gpio.number, level_.percent(), level.percent(),
            level_.percent(), ActuatorOperationResult::NotInitialized, "set");
        return ActuatorOperationResult::NotInitialized;
    }
    const uint8_t previous = level_.percent();
    ledcWrite(channel_, gpioPwmDutyForLevel(level));
    level_ = level;
    if (previous != level_.percent()) recordActuatorOutput(diagnosticId_, hardware_.gpio.number,
        previous, level.percent(), level_.percent(), ActuatorOperationResult::Completed, "set");
    return ActuatorOperationResult::Completed;
}

ActuatorLevel GpioPwmActuator::level() const {
    return level_;
}

ActuatorOperationResult GpioPwmActuator::shutdown() {
    if (!initialized_) return ActuatorOperationResult::Completed;
    const uint8_t previous = outputPercent();
    ledcWrite(channel_, 0);
    level_ = ActuatorLevel::off();
    ledcDetachPin(hardware_.gpio.number);
    pinMode(hardware_.gpio.number, INPUT);
    initialized_ = false;
    recordActuatorOutput(diagnosticId_, hardware_.gpio.number, previous, 0, 0,
        ActuatorOperationResult::Completed, "shutdown");
    logger_.debugf("GPIO PWM actuator shut down Off on GPIO%u channel=%u",
        hardware_.gpio.number, static_cast<unsigned int>(channel_));
    return ActuatorOperationResult::Completed;
}

bool GpioPwmActuator::initialized() const {
    return initialized_;
}

} // namespace EnvNode
