#pragma once

#include "RuntimeAction.h"

namespace WeatherStation {

enum class ConfigurationArea {
    Network,
    Mqtt,
    Time,
    Locale,
    PresentationUnits,
    Device,
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

} // namespace WeatherStation
