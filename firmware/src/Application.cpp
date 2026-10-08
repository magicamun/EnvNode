#include "Application.h"
#include "FirmwareBuildInfo.h"
#include <Arduino.h>

namespace EnvNode {

Application::Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime, ControllerRuntime& controllerRuntime, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher, MqttMessageRouter& mqttMessageRouter, ActuatorMqttAdapter& actuatorMqttAdapter, ActuatorStatePublisher& actuatorStatePublisher, ControllerMqttAdapter& controllerMqttAdapter, ControllerStatePublisher& controllerStatePublisher, MqttDescriptionPublisher& descriptionPublisher, DisplayService& displayService, ValueMqttAdapter& valueMqttAdapter, ValueStatePublisher& valueStatePublisher)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService)
    , webService_(webService)
    , mqttService_(mqttService)
    , timeService_(timeService)
    , sensorManager_(sensorManager)
    , actuatorRuntime_(actuatorRuntime)
    , controllerRuntime_(controllerRuntime)
    , runtimeManager_(runtimeManager)
    , discoveryPublisher_(discoveryPublisher)
    , mqttMessageRouter_(mqttMessageRouter)
    , actuatorMqttAdapter_(actuatorMqttAdapter)
    , actuatorStatePublisher_(actuatorStatePublisher)
    , controllerMqttAdapter_(controllerMqttAdapter)
    , controllerStatePublisher_(controllerStatePublisher)
    , descriptionPublisher_(descriptionPublisher)
    , displayService_(displayService)
    , valueMqttAdapter_(valueMqttAdapter), valueStatePublisher_(valueStatePublisher) {
}

void Application::setup(bool configurationAlreadyLoaded) {
    if (!configurationAlreadyLoaded) configurationService_.loadConfiguration();
    wifiService_.begin();
    timeService_.begin();
    sensorManager_.begin();
    actuatorRuntime_.initialize(configurationService_.getConfiguration().actuatorSlots);
    if (!controllerRuntime_.initialize(
            configurationService_.getConfiguration().controllerSlots)) {
        logger_.error("Controller runtime initialization failed");
    }
    webService_.begin();
    mqttService_.begin();
    mqttMessageRouter_.begin();
    actuatorMqttAdapter_.begin();
    controllerMqttAdapter_.begin();

    const Configuration& configuration = configurationService_.getConfiguration();

    logger_.info(configuration.device.name.c_str());
    logger_.infof("Firmware version: %s", FirmwareBuildInfo::SemanticVersion);
    logger_.infof("Build: %s", FirmwareBuildInfo::BuildNumber);
    logger_.infof("Git commit: %s", FirmwareBuildInfo::GitCommit);
    logger_.debugf("Git branch: %s", FirmwareBuildInfo::GitBranch);
    logger_.debugf("Git working tree: %s", FirmwareBuildInfo::SourceState);
    logger_.debugf("Built: %s", FirmwareBuildInfo::BuildTimestampUtc);
    logger_.debugf("Chip model: %s", ESP.getChipModel());
    logger_.debugf("CPU frequency: %u MHz", ESP.getCpuFreqMHz());
    logger_.debugf("Flash size: %u KB", ESP.getFlashChipSize() / 1024);
    logger_.debugf("Free heap: %u bytes", ESP.getFreeHeap());
}

void Application::loop() {
    wifiService_.loop();
    timeService_.loop();

    sensorManager_.loop();
    webService_.loop();
    mqttService_.loop();
    actuatorMqttAdapter_.loop();
    controllerMqttAdapter_.loop();
    valueMqttAdapter_.loop();
    valueStatePublisher_.loop();
    controllerRuntime_.loop();
    actuatorStatePublisher_.loop();
    controllerStatePublisher_.loop();
    descriptionPublisher_.loop();
    discoveryPublisher_.loop();
    runtimeManager_.service();
    displayService_.loop();
}

} // namespace EnvNode
