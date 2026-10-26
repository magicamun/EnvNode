#pragma once

#include "FirmwareRuntime.h"
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
#include "MqttDescriptionPublisher.h"
#include "DisplayService.h"
#include "ValueMqttAdapter.h"
#include "ValueStatePublisher.h"

namespace EnvNode {

class Application : private IFirmwareApplication {
public:
    Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime, ControllerRuntime& controllerRuntime, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher, MqttMessageRouter& mqttMessageRouter, ActuatorMqttAdapter& actuatorMqttAdapter, ActuatorStatePublisher& actuatorStatePublisher, ControllerMqttAdapter& controllerMqttAdapter, ControllerStatePublisher& controllerStatePublisher, MqttDescriptionPublisher& descriptionPublisher, DisplayService& displayService, ValueMqttAdapter& valueMqttAdapter, ValueStatePublisher& valueStatePublisher);

    void setup(bool configurationAlreadyLoaded = false);
    void loop();

private:
    void initializeDevices() override;
    void beginIntegrations() override;
    void serviceMeasurements() override;
    void serviceIntegrations() override;
    FirmwareRuntime firmwareRuntime_;
    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
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
    MqttDescriptionPublisher& descriptionPublisher_;
    DisplayService& displayService_;
    ValueMqttAdapter& valueMqttAdapter_;
    ValueStatePublisher& valueStatePublisher_;
};

} // namespace EnvNode
