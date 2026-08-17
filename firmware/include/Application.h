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
#include "ActuatorRuntime.h"
#include "ActuatorMqttAdapter.h"
#include "ActuatorStatePublisher.h"

namespace EnvNode {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher, ActuatorMqttAdapter& actuatorMqttAdapter, ActuatorStatePublisher& actuatorStatePublisher);

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
    ActuatorRuntime& actuatorRuntime_;
    RuntimeManager& runtimeManager_;
    IDiscoveryPublisher& discoveryPublisher_;
    ActuatorMqttAdapter& actuatorMqttAdapter_;
    ActuatorStatePublisher& actuatorStatePublisher_;
    bool timeSyncLogged_ = false;
};

} // namespace EnvNode
