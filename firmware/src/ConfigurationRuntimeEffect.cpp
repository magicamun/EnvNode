#include "ConfigurationRuntimeEffect.h"

namespace EnvNode {

RuntimeAction runtimeActionFor(ConfigurationArea area) {
    switch (area) {
        case ConfigurationArea::Network: return RuntimeAction::RestartDevice;
        case ConfigurationArea::Mqtt: return RuntimeAction::RestartMqtt;
        case ConfigurationArea::Time: return RuntimeAction::RestartTime;
        case ConfigurationArea::Locale: return RuntimeAction::None;
        case ConfigurationArea::PresentationUnits: return RuntimeAction::None;
        case ConfigurationArea::Device:
            // The device name contributes to the MQTT client identity on connect.
            return RuntimeAction::RestartMqtt;
        case ConfigurationArea::Sensors: return RuntimeAction::RestartSensorManager;
        case ConfigurationArea::Actuators: return RuntimeAction::RestartActuatorRuntime;
        case ConfigurationArea::Controllers: return RuntimeAction::RestartControllerRuntime;
        default: return RuntimeAction::None;
    }
}

ConfigurationSaveResult configurationSaveResult(bool success, ConfigurationArea area) {
    return {success, success ? runtimeActionFor(area) : RuntimeAction::None};
}

ConfigurationSaveResult configurationSaveResult(
    bool success,
    ConfigurationArea firstArea,
    bool firstChanged,
    ConfigurationArea secondArea,
    bool secondChanged) {
    if (!success) return {false, RuntimeAction::None};
    const RuntimeAction firstAction = firstChanged
        ? runtimeActionFor(firstArea) : RuntimeAction::None;
    const RuntimeAction secondAction = secondChanged
        ? runtimeActionFor(secondArea) : RuntimeAction::None;
    return {
        true,
        static_cast<uint8_t>(firstAction) >= static_cast<uint8_t>(secondAction)
            ? firstAction : secondAction,
    };
}

} // namespace EnvNode
