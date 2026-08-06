#pragma once

#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "Logger.h"

namespace WeatherStation {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService);

    void setup();
    void loop();

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
};

} // namespace WeatherStation
