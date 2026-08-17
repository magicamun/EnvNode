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
#include "ControllerRuntime.h"
#include "ControllerMqttAdapter.h"
#include "ControllerStatePublisher.h"
#include "MqttMessageRouter.h"

namespace EnvNode {

class Application {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime, ControllerRuntime& controllerRuntime, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher, MqttMessageRouter& mqttMessageRouter, ActuatorMqttAdapter& actuatorMqttAdapter, ActuatorStatePublisher& actuatorStatePublisher, ControllerMqttAdapter& controllerMqttAdapter, ControllerStatePublisher& controllerStatePublisher);

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
    ControllerRuntime& controllerRuntime_;
    RuntimeManager& runtimeManager_;
    IDiscoveryPublisher& discoveryPublisher_;
    MqttMessageRouter& mqttMessageRouter_;
    ActuatorMqttAdapter& actuatorMqttAdapter_;
    ActuatorStatePublisher& actuatorStatePublisher_;
    ControllerMqttAdapter& controllerMqttAdapter_;
    ControllerStatePublisher& controllerStatePublisher_;
    bool timeSyncLogged_ = false;
};

} // namespace EnvNode
