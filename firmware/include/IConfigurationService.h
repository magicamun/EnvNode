#pragma once

#include "Configuration.h"
#include "INetworkSettings.h"

namespace EnvNode {

class IConfigurationService : public INetworkSettings {
public:
    virtual ~IConfigurationService() = default;

    virtual void loadConfiguration() = 0;
    virtual const Configuration& getConfiguration() const = 0;
    virtual Locale getLocale() const = 0;
    const NetworkConfiguration& getNetworkConfiguration() const override {
        return getConfiguration().network;
    }

    virtual bool setPressureReduction(const PressureReductionConfiguration& configuration) { return false; }
    virtual bool setGeoPosition(const GeoPosition& position) { return false; }
    virtual bool setDeviceName(const String& deviceName) = 0;
    virtual bool setNetworkConfiguration(
        const NetworkConfiguration& network,
        bool updatePassword) override = 0;
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
    virtual bool setControllerSlotConfiguration(const ControllerSlotConfiguration& slot) = 0;
    virtual bool setDisplayConfiguration(const DisplayConfiguration& display) = 0;
    virtual bool setEnumValueDefinitions(const std::vector<EnumValueConfiguration>& definitions) = 0;
    virtual bool saveEnumValueCode(ValueId id, const String& code) = 0;
    virtual bool resetToDefaults() = 0;
    virtual bool validateRestore(const Configuration& candidate) const { return false; }
    virtual bool restoreConfiguration(const Configuration& candidate) { return false; }
};

} // namespace EnvNode
