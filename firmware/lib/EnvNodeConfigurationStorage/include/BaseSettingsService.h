// EnvNode: see repository LICENSE.
#pragma once
#include "ConfigurationStorage.h"
#include "IBaseSettings.h"
#include "INetworkSettings.h"
namespace EnvNode {
struct BaseSettingsDefaults {
    const char* deviceName;
    const char* hostname;
    const char* timezone;
    const char* ntpServer1;
    const char* ntpServer2;
    Locale locale;
};
// References and defaults must outlive this adapter. No EnvNode model is required.
class BaseSettingsService : public IBaseSettings, public INetworkSettings {
public:
    BaseSettingsService(ConfigurationStorage& storage, DeviceConfiguration& device,
        NetworkConfiguration& network, TimeConfiguration& time, LocaleConfiguration& locale,
        const BaseSettingsDefaults& defaults);
    const DeviceConfiguration& getDeviceConfiguration() const override { return device_; }
    const NetworkConfiguration& getNetworkConfiguration() const override { return network_; }
    const TimeConfiguration& getTimeConfiguration() const override { return time_; }
    Locale getLocale() const override { return locale_.locale; }
    void initializeDefaults();
    void loadIdentityAndNetworkOverrides();
    void loadTimeAndLocaleOverrides();
    void validateIdentityAndNetwork();
    void validateTime();
    bool setDeviceName(const String& deviceName) override;
    bool setNetworkConfiguration(const NetworkConfiguration& network, bool updatePassword) override;
    bool setWifiSSID(const String& ssid);
    bool setWifiPassword(const String& password);
    bool setTimezone(const String& timezone) override;
    bool setNtpServer1(const String& server) override;
    bool setNtpServer2(const String& server) override;
    bool setLocale(Locale locale) override;
private:
    ConfigurationStorage& storage_;
    DeviceConfiguration& device_;
    NetworkConfiguration& network_;
    TimeConfiguration& time_;
    LocaleConfiguration& locale_;
    const BaseSettingsDefaults& defaults_;
};
}
