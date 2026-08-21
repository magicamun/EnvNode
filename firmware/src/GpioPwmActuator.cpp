#include "GpioPwmActuator.h"

#include <Arduino.h>

namespace EnvNode {

uint32_t gpioPwmDutyForLevel(ActuatorLevel level) {
    return (static_cast<uint32_t>(level.percent()) * GpioPwmMaximumDuty + 50U) / 100U;
}

GpioPwmActuator::GpioPwmActuator(
    const HardwareResourceAssignment& hardware,
    uint8_t channel,
    ILogger& logger)
    : hardware_(hardware)
    , channel_(channel)
    , logger_(logger) {
}

ActuatorOperationResult GpioPwmActuator::begin() {
    const HardwareResourceValidationResult validation = BoardCapabilities::current().validate(
        HardwareInterfaceKind::GPIO,
        hardware_,
        GpioCapability::DigitalOutput);
    if (validation != HardwareResourceValidationResult::Valid) {
        initialized_ = false;
        logger_.errorf(
            "GPIO PWM actuator initialization failed: invalid hardware resource (%u)",
            static_cast<unsigned int>(validation));
        return ActuatorOperationResult::InvalidHardwareResource;
    }
    if (ledcSetup(channel_, GpioPwmFrequencyHz, GpioPwmResolutionBits) == 0) {
        initialized_ = false;
        logger_.errorf("GPIO PWM actuator initialization failed: LEDC channel %u",
            static_cast<unsigned int>(channel_));
        return ActuatorOperationResult::InvalidHardwareResource;
    }

    ledcWrite(channel_, 0);
    ledcAttachPin(hardware_.gpio.number, channel_);
    level_ = ActuatorLevel::off();
    initialized_ = true;
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
    if (!initialized_) return ActuatorOperationResult::NotInitialized;
    ledcWrite(channel_, gpioPwmDutyForLevel(level));
    level_ = level;
    return ActuatorOperationResult::Completed;
}

ActuatorLevel GpioPwmActuator::level() const {
    return level_;
}

ActuatorOperationResult GpioPwmActuator::shutdown() {
    if (!initialized_) return ActuatorOperationResult::Completed;
    ledcWrite(channel_, 0);
    level_ = ActuatorLevel::off();
    ledcDetachPin(hardware_.gpio.number);
    pinMode(hardware_.gpio.number, INPUT);
    initialized_ = false;
    logger_.debugf("GPIO PWM actuator shut down Off on GPIO%u channel=%u",
        hardware_.gpio.number, static_cast<unsigned int>(channel_));
    return ActuatorOperationResult::Completed;
}

bool GpioPwmActuator::initialized() const {
    return initialized_;
}

} // namespace EnvNode
