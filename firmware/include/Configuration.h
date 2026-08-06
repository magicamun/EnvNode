#pragma once

#include <Arduino.h>

namespace WeatherStation {

struct Configuration {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String mqttServer;
    uint16_t mqttPort;
};

} // namespace WeatherStation
