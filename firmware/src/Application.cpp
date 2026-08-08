#include "Application.h"
#include "FirmwareVersion.h"
#include <Arduino.h>

namespace WeatherStation {

Application::Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService, IMqttService& mqttService, ITimeService& timeService, SensorManager& sensorManager, RuntimeManager& runtimeManager)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService)
    , webService_(webService)
    , mqttService_(mqttService)
    , timeService_(timeService)
    , sensorManager_(sensorManager)
    , runtimeManager_(runtimeManager)
    , timeSyncLogged_(false) {
}

void Application::setup(bool configurationAlreadyLoaded) {
    logger_.begin(115200);
    delay(500);

    if (!configurationAlreadyLoaded) configurationService_.loadConfiguration();
    wifiService_.begin();
    timeService_.begin();
    sensorManager_.begin();
    webService_.begin();
    mqttService_.begin();

    const Configuration& configuration = configurationService_.getConfiguration();

    logger_.println(configuration.device.name.c_str());
    logger_.printf("Firmware version: %s\n", FirmwareVersion);
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
    runtimeManager_.service();
}

} // namespace WeatherStation
