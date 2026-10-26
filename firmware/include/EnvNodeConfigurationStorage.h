// EnvNode: see repository LICENSE.
#pragma once
#include "ConfigurationStorage.h"
namespace EnvNode {
constexpr ConfigurationStorageLayout EnvNodeConfigurationStorage = {
    "weather", "weather0", "weather1", "weather_meta", "cfg_nvs"
};
}
