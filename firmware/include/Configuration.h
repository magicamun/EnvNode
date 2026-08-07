#pragma once

#include <Arduino.h>

namespace WeatherStation {

struct Configuration {
    String deviceName;
    String wifiSSID;
    String wifiPassword;
    String mqttServer;
    uint16_t mqttPort;
    String mqttUsername;
    String mqttPassword;
    String timezone;
    String ntpServer1;
    String ntpServer2;
};

} // namespace WeatherStation
