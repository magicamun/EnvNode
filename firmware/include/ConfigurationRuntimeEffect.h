#pragma once

#include "RuntimeAction.h"

namespace EnvNode {

enum class ConfigurationArea {
    Network,
    Mqtt,
    Time,
    Locale,
    PresentationUnits,
    Device,
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
