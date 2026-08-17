#include "GpioOnOffActuator.h"

#include <Arduino.h>

namespace EnvNode {

GpioOnOffActuator::GpioOnOffActuator(
    const HardwareResourceAssignment& hardware,
    ILogger& logger)
    : hardware_(hardware)
    , logger_(logger) {
}

ActuatorOperationResult GpioOnOffActuator::begin() {
    const HardwareResourceValidationResult validation = BoardCapabilities::current().validate(
        HardwareInterfaceKind::GPIO,
        hardware_,
        GpioCapability::DigitalOutput);
    if (validation != HardwareResourceValidationResult::Valid) {
        initialized_ = false;
        logger_.printf(
            "GPIO OnOff actuator initialization failed: invalid hardware resource (%u)\n",
            static_cast<unsigned int>(validation));
        return ActuatorOperationResult::InvalidHardwareResource;
    }

    pinMode(hardware_.gpio.number, OUTPUT);
    digitalWrite(hardware_.gpio.number, LOW);
    state_ = OnOffState::Off;
    initialized_ = true;
    logger_.printf("GPIO OnOff actuator initialized Off on GPIO%u\n", hardware_.gpio.number);
    return ActuatorOperationResult::Completed;
}

ActuatorOperationResult GpioOnOffActuator::setState(OnOffState state) {
    if (!initialized_) return ActuatorOperationResult::NotInitialized;
    digitalWrite(hardware_.gpio.number, state == OnOffState::On ? HIGH : LOW);
    state_ = state;
    return ActuatorOperationResult::Completed;
}

ActuatorOperationResult GpioOnOffActuator::shutdown() {
    if (!initialized_) return ActuatorOperationResult::Completed;
    digitalWrite(hardware_.gpio.number, LOW);
    state_ = OnOffState::Off;
    pinMode(hardware_.gpio.number, INPUT);
    initialized_ = false;
    logger_.printf("GPIO OnOff actuator shut down Off on GPIO%u\n", hardware_.gpio.number);
    return ActuatorOperationResult::Completed;
}

OnOffState GpioOnOffActuator::state() const {
    return state_;
}

bool GpioOnOffActuator::initialized() const {
    return initialized_;
}

} // namespace EnvNode
