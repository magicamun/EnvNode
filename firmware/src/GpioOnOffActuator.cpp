#include "GpioOnOffActuator.h"

#include <Arduino.h>
#include "ActuatorDiagnostics.h"

namespace EnvNode {

GpioOnOffActuator::GpioOnOffActuator(
    const HardwareResourceAssignment& hardware,
    ILogger& logger, ActuatorId diagnosticId)
    : hardware_(hardware)
    , logger_(logger)
    , diagnosticId_(diagnosticId) {
}

ActuatorOperationResult GpioOnOffActuator::begin() {
    const uint8_t previous = outputPercent();
    const HardwareResourceValidationResult validation = BoardCapabilities::current().validate(
        HardwareInterfaceKind::GPIO,
        hardware_,
        GpioCapability::DigitalOutput);
    if (validation != HardwareResourceValidationResult::Valid) {
        initialized_ = false;
        logger_.errorf(
            "GPIO OnOff actuator initialization failed: invalid hardware resource (%u)",
            static_cast<unsigned int>(validation));
        recordActuatorOutput(diagnosticId_, hardware_.gpio.number, outputPercent(), 0, outputPercent(),
            ActuatorOperationResult::InvalidHardwareResource, "begin");
        return ActuatorOperationResult::InvalidHardwareResource;
    }

    pinMode(hardware_.gpio.number, OUTPUT);
    digitalWrite(hardware_.gpio.number, LOW);
    state_ = OnOffState::Off;
    initialized_ = true;
    recordActuatorOutput(diagnosticId_, hardware_.gpio.number, previous, 0, 0,
        ActuatorOperationResult::Completed, "begin");
    logger_.debugf("GPIO OnOff actuator initialized Off on GPIO%u", hardware_.gpio.number);
    return ActuatorOperationResult::Completed;
}

ActuatorOperationResult GpioOnOffActuator::setState(OnOffState state) {
    if (!initialized_) {
        recordActuatorOutput(diagnosticId_, hardware_.gpio.number, outputPercent(),
            state == OnOffState::On ? 100 : 0, outputPercent(),
            ActuatorOperationResult::NotInitialized, "set");
        return ActuatorOperationResult::NotInitialized;
    }
    const uint8_t previous = outputPercent();
    digitalWrite(hardware_.gpio.number, state == OnOffState::On ? HIGH : LOW);
    state_ = state;
    if (previous != outputPercent()) recordActuatorOutput(diagnosticId_, hardware_.gpio.number,
        previous, outputPercent(), outputPercent(), ActuatorOperationResult::Completed, "set");
    return ActuatorOperationResult::Completed;
}

ActuatorOperationResult GpioOnOffActuator::shutdown() {
    if (!initialized_) return ActuatorOperationResult::Completed;
    const uint8_t previous = outputPercent();
    digitalWrite(hardware_.gpio.number, LOW);
    state_ = OnOffState::Off;
    pinMode(hardware_.gpio.number, INPUT);
    initialized_ = false;
    recordActuatorOutput(diagnosticId_, hardware_.gpio.number, previous, 0, 0,
        ActuatorOperationResult::Completed, "shutdown");
    logger_.debugf("GPIO OnOff actuator shut down Off on GPIO%u", hardware_.gpio.number);
    return ActuatorOperationResult::Completed;
}

OnOffState GpioOnOffActuator::state() const {
    return state_;
}

bool GpioOnOffActuator::initialized() const {
    return initialized_;
}

} // namespace EnvNode
