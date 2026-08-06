#pragma once

#include "IConfigurationService.h"
#include "Logger.h"

namespace WeatherStation {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService);

    void setup();
    void loop();

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
};

} // namespace WeatherStation
