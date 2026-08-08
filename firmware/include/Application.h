#pragma once

#include "IConfigurationService.h"
#include "IWebService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "RuntimeManager.h"
#include "Logger.h"
#include "IDiscoveryPublisher.h"

namespace WeatherStation {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher);

    void setup(bool configurationAlreadyLoaded = false);
    void loop();

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IWebService& webService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    SensorManager& sensorManager_;
    RuntimeManager& runtimeManager_;
    IDiscoveryPublisher& discoveryPublisher_;
    bool timeSyncLogged_ = false;
};

} // namespace WeatherStation
