#pragma once

#include "Configuration.h"

namespace EnvNode {

class IConfigurationService {
public:
    virtual ~IConfigurationService() = default;

    virtual void loadConfiguration() = 0;
    virtual const Configuration& getConfiguration() const = 0;
    virtual Locale getLocale() const = 0;

    virtual bool setDeviceName(const String& deviceName) = 0;
    virtual bool setNetworkConfiguration(
        const NetworkConfiguration& network,
        bool updatePassword) = 0;
    virtual bool setWifiSSID(const String& ssid) = 0;
    virtual bool setWifiPassword(const String& password) = 0;
    virtual bool setMqttServer(const String& server) = 0;
    virtual bool setMqttPort(uint16_t port) = 0;
    virtual bool setMqttUsername(const String& username) = 0;
    virtual bool setMqttPassword(const String& password) = 0;
    virtual bool setTimezone(const String& timezone) = 0;
    virtual bool setNtpServer1(const String& server) = 0;
    virtual bool setNtpServer2(const String& server) = 0;
    virtual bool setLocale(Locale locale) = 0;
    virtual bool setPresentationUnit(MeasurementType type, PresentationUnit unit) = 0;
    virtual bool setSensorSlotConfiguration(const SensorSlotConfiguration& slot) = 0;
    virtual bool setActuatorSlotConfiguration(const ActuatorSlotConfiguration& slot) = 0;
    virtual bool resetToDefaults() = 0;
};

} // namespace EnvNode
