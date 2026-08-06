#include "DefaultConfigurationService.h"

namespace WeatherStation {

void DefaultConfigurationService::loadConfiguration() {
    configuration_ = Configuration{
        .deviceName = "WeatherStation",
    };
}

const Configuration& DefaultConfigurationService::getConfiguration() const {
    return configuration_;
}

} // namespace WeatherStation
