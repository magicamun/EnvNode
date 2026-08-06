#pragma once

#include "Configuration.h"

namespace WeatherStation {

class IConfigurationService {
public:
    virtual ~IConfigurationService() = default;

    virtual void loadConfiguration() = 0;
    virtual const Configuration& getConfiguration() const = 0;
};

} // namespace WeatherStation
