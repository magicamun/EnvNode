#pragma once

#include "RuntimeAction.h"

namespace EnvNode {

struct SensorSlotConfiguration;
struct ActuatorSlotConfiguration;
struct ControllerSlotConfiguration;

// Compare effective slot settings, excluding the live diagnostic selection.
RuntimeAction runtimeActionForChange(const SensorSlotConfiguration& before,const SensorSlotConfiguration& after);
RuntimeAction runtimeActionForChange(const ActuatorSlotConfiguration& before,const ActuatorSlotConfiguration& after);
RuntimeAction runtimeActionForChange(const ControllerSlotConfiguration& before,const ControllerSlotConfiguration& after);

enum class ConfigurationArea {
    Network,
    Mqtt,
    Time,
    Locale,
    PresentationUnits,
    Device,
    GeoPosition,
    Sensors,
    Actuators,
    Controllers,
};

struct ConfigurationSaveResult {
    bool success;
    RuntimeAction requiredAction;
};

RuntimeAction runtimeActionFor(ConfigurationArea area);
ConfigurationSaveResult configurationSaveResult(bool success, ConfigurationArea area);
ConfigurationSaveResult configurationSaveResult(
    bool success,
    ConfigurationArea firstArea,
    bool firstChanged,
    ConfigurationArea secondArea,
    bool secondChanged);

} // namespace EnvNode
