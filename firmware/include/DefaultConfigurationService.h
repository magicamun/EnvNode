#pragma once

#include "ConfigurationService.h"

namespace WeatherStation {

class DefaultConfigurationService : public IConfigurationService {
public:
    void loadConfiguration() override;
    const Configuration& getConfiguration() const override;

private:
    Configuration configuration_;
};

} // namespace WeatherStation
