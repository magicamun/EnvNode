#include "Application.h"
#include "FirmwareBuildInfo.h"
#include <Arduino.h>

namespace EnvNode {

Application::Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, ActuatorRuntime& actuatorRuntime, ControllerRuntime& controllerRuntime, RuntimeManager& runtimeManager, IDiscoveryPublisher& discoveryPublisher, MqttMessageRouter& mqttMessageRouter, ActuatorMqttAdapter& actuatorMqttAdapter, ActuatorStatePublisher& actuatorStatePublisher, ControllerMqttAdapter& controllerMqttAdapter, ControllerStatePublisher& controllerStatePublisher, MqttDescriptionPublisher& descriptionPublisher)
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
    , timeSyncLogged_(false) {
}

void Application::setup(bool configurationAlreadyLoaded) {
    logger_.begin(115200);
    delay(500);

    if (!configurationAlreadyLoaded) configurationService_.loadConfiguration();
    wifiService_.begin();
    timeService_.begin();
    sensorManager_.begin();
    actuatorRuntime_.initialize(configurationService_.getConfiguration().actuatorSlots);
    if (!controllerRuntime_.initialize(
            configurationService_.getConfiguration().controllerSlots)) {
        logger_.println("Controller runtime initialization failed");
    }
    webService_.begin();
    mqttService_.begin();
    mqttMessageRouter_.begin();
    actuatorMqttAdapter_.begin();
    controllerMqttAdapter_.begin();

    const Configuration& configuration = configurationService_.getConfiguration();

    logger_.println(configuration.device.name.c_str());
    logger_.printf("Firmware version: %s\n", FirmwareBuildInfo::SemanticVersion);
    logger_.printf("Build: %s\n", FirmwareBuildInfo::BuildNumber);
    logger_.printf("Git commit: %s\n", FirmwareBuildInfo::GitCommit);
    logger_.printf("Git branch: %s\n", FirmwareBuildInfo::GitBranch);
    logger_.printf("Git working tree: %s\n", FirmwareBuildInfo::SourceState);
    logger_.printf("Built: %s\n", FirmwareBuildInfo::BuildTimestampUtc);
    logger_.printf("Chip model: %s\n", ESP.getChipModel());
    logger_.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
    logger_.printf("Flash size: %u KB\n", ESP.getFlashChipSize() / 1024);
    logger_.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

void Application::loop() {
    wifiService_.loop();
    timeService_.loop();

    if (!timeSyncLogged_ && timeService_.synchronized()) {
        logger_.println("Time synchronized");
        timeSyncLogged_ = true;
    }

    sensorManager_.loop();
    webService_.loop();
    mqttService_.loop();
    actuatorMqttAdapter_.loop();
    controllerMqttAdapter_.loop();
    controllerRuntime_.loop();
    actuatorStatePublisher_.loop();
    controllerStatePublisher_.loop();
    descriptionPublisher_.loop();
    discoveryPublisher_.loop();
    runtimeManager_.service();
}

} // namespace EnvNode
