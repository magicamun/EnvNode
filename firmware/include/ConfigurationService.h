#pragma once

#include <Preferences.h>
#include "Configuration.h"
#include "IConfigurationService.h"

namespace WeatherStation {

class ConfigurationService : public IConfigurationService {
public:
    void loadConfiguration() override;
    const Configuration& getConfiguration() const override;

    bool setDeviceName(const String& deviceName) override;
    bool setWifiSSID(const String& ssid) override;
    bool setWifiPassword(const String& password) override;
    bool setMqttServer(const String& server) override;
    bool setMqttPort(uint16_t port) override;
    bool setMqttUsername(const String& username) override;
    bool setMqttPassword(const String& password) override;
    bool setTimezone(const String& timezone) override;
    bool setNtpServer1(const String& server) override;
    bool setNtpServer2(const String& server) override;
    bool resetToDefaults() override;

private:
    void initializeDefaults();
    void loadFromPreferences();
    void validateConfiguration();
    void ensurePreferencesStarted();
    bool persistString(const char* key, const String& value);
    bool persistUInt(const char* key, uint32_t value);
    bool validateDeviceName(const String& deviceName) const;
    bool validateWifiSSID(const String& ssid) const;
    bool validateWifiPassword(const String& password) const;
    bool validateMqttServer(const String& server) const;
    bool validateMqttPort(uint16_t port) const;
    bool validateMqttUsername(const String& username) const;
    bool validateMqttPassword(const String& password) const;
    bool validateTimezone(const String& timezone) const;
    bool validateNtpServer(const String& server) const;

    Configuration configuration_;
    Preferences preferences_;
    bool preferencesInitialized_ = false;
};

} // namespace WeatherStation
