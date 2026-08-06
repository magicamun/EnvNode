#include "Application.h"
#include <Arduino.h>

namespace WeatherStation {

constexpr auto CurrentFirmwareVersion = "0.1.0";

Application::Application(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService, IWebService& webService)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService)
    , webService_(webService) {
}

void Application::setup() {
    logger_.begin(115200);
    delay(500);

    configurationService_.loadConfiguration();
    wifiService_.begin();
    webService_.begin();

    const Configuration& configuration = configurationService_.getConfiguration();

    logger_.println(configuration.deviceName.c_str());
    logger_.printf("Firmware version: %s\n", CurrentFirmwareVersion);
    logger_.printf("Chip model: %s\n", ESP.getChipModel());
    logger_.printf("CPU frequency: %u MHz\n", ESP.getCpuFreqMHz());
    logger_.printf("Flash size: %u KB\n", ESP.getFlashChipSize() / 1024);
    logger_.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
}

void Application::loop() {
    wifiService_.loop();
    webService_.loop();
}

} // namespace WeatherStation
