#pragma once
#include "Configuration.h"
namespace EnvNode {
String buildSelectorFields(const Configuration& configuration, const ControllerSlotConfiguration& slot,
    const String& targetOptions);
}
