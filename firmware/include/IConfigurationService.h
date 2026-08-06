#pragma once

#include "Configuration.h"

namespace WeatherStation {

class IConfigurationService {
public:
    virtual ~IConfigurationService() = default;

    virtual void loadConfiguration() = 0;
    virtual const Configuration& getConfiguration() const = 0;

    virtual bool setDeviceName(const String& deviceName) = 0;
    virtual bool setWifiSSID(const String& ssid) = 0;
    virtual bool setWifiPassword(const String& password) = 0;
    virtual bool setMqttServer(const String& server) = 0;
    virtual bool setMqttPort(uint16_t port) = 0;
};

} // namespace WeatherStation
