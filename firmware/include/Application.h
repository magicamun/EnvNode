#pragma once

#include "IConfigurationService.h"
#include "IWebService.h"
#include "IWiFiService.h"
#include "IMqttService.h"
#include "ITimeService.h"
#include "SensorManager.h"
#include "Logger.h"

namespace WeatherStation {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager);

    void setup();
    void loop();

private:
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IWebService& webService_;
    IMqttService& mqttService_;
    ITimeService& timeService_;
    SensorManager& sensorManager_;
    bool timeSyncLogged_ = false;
};

} // namespace WeatherStation
