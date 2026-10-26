// EnvNode: see repository LICENSE.
#include "BaseSettingsService.h"
#include "BaseConfigurationValidation.h"
namespace EnvNode {
namespace {
constexpr const char* KeyDeviceName = "deviceName";
constexpr const char* KeyWifiSSID = "wifiSSID";
constexpr const char* KeyWifiPassword = "wifiPassword";
constexpr const char* KeyHostname = "hostname";
constexpr const char* KeyAddressMode = "addressMode";
constexpr const char* KeyIpv4Address = "ipv4Address";
constexpr const char* KeySubnetMask = "subnetMask";
constexpr const char* KeyGateway = "gateway";
constexpr const char* KeyDns1 = "dns1";
constexpr const char* KeyDns2 = "dns2";
constexpr const char* KeyTimezone = "timezone";
constexpr const char* KeyNtpServer1 = "ntpServer1";
constexpr const char* KeyNtpServer2 = "ntpServer2";
constexpr const char* KeyLocale = "locale";
}
BaseSettingsService::BaseSettingsService(ConfigurationStorage& storage, DeviceConfiguration& device,
    NetworkConfiguration& network, TimeConfiguration& time, LocaleConfiguration& locale,
    const BaseSettingsDefaults& defaults)
    : storage_(storage), device_(device), network_(network), time_(time), locale_(locale), defaults_(defaults) {}
void BaseSettingsService::initializeDefaults() {
    device_.name = defaults_.deviceName;
    network_.hostname = defaults_.hostname;
    network_.wifiSSID = String();
    network_.wifiPassword = String();
    network_.addressMode = NetworkAddressMode::Dhcp;
    network_.ipv4Address = String();
    network_.subnetMask = String();
    network_.gateway = String();
    network_.dns1 = String();
    network_.dns2 = String();
    time_.timezone = String(defaults_.timezone);
    time_.ntpServer1 = String(defaults_.ntpServer1);
    time_.ntpServer2 = String(defaults_.ntpServer2);
    locale_.locale = defaults_.locale;
}
void BaseSettingsService::loadIdentityAndNetworkOverrides() {
    storage_.begin();
    if (storage_.preferences().isKey(KeyDeviceName)) {
        device_.name = storage_.preferences().getString(KeyDeviceName, device_.name);
    }
    if (storage_.preferences().isKey(KeyWifiSSID)) {
        network_.wifiSSID = storage_.preferences().getString(KeyWifiSSID, network_.wifiSSID);
    }
    if (storage_.preferences().isKey(KeyWifiPassword)) {
        network_.wifiPassword = storage_.preferences().getString(KeyWifiPassword, network_.wifiPassword);
    }
    network_.hostname = storage_.preferences().getString(KeyHostname, storage_.preferences().isKey("cfg_snapshot")
        ? network_.hostname : device_.name);
    const uint32_t addressMode = storage_.preferences().getUInt(KeyAddressMode, static_cast<uint32_t>(network_.addressMode));
    network_.addressMode = addressMode == 1
        ? NetworkAddressMode::Static : NetworkAddressMode::Dhcp;
    network_.ipv4Address = storage_.preferences().getString(KeyIpv4Address, network_.ipv4Address);
    network_.subnetMask = storage_.preferences().getString(KeySubnetMask, network_.subnetMask);
    network_.gateway = storage_.preferences().getString(KeyGateway, network_.gateway);
    network_.dns1 = storage_.preferences().getString(KeyDns1, network_.dns1);
    if (storage_.preferences().isKey(KeyDns2)) {
        network_.dns2 = storage_.preferences().getString(KeyDns2, "");
    }

}
void BaseSettingsService::loadTimeAndLocaleOverrides() {
    storage_.begin();
    if (storage_.preferences().isKey(KeyTimezone)) {
        time_.timezone = storage_.preferences().getString(KeyTimezone, time_.timezone);
    }
    if (storage_.preferences().isKey(KeyNtpServer1)) {
        time_.ntpServer1 = storage_.preferences().getString(KeyNtpServer1, time_.ntpServer1);
    }
    if (storage_.preferences().isKey(KeyNtpServer2)) {
        time_.ntpServer2 = storage_.preferences().getString(KeyNtpServer2, time_.ntpServer2);
    }
    const uint32_t storedLocale = storage_.preferences().getUInt(
        KeyLocale, static_cast<uint32_t>(locale_.locale));
    locale_.locale = storedLocale <= static_cast<uint32_t>(Locale::EnglishUnitedStates)
        ? static_cast<Locale>(storedLocale)
        : Locale::GermanGermany;

}
void BaseSettingsService::validateIdentityAndNetwork() {
    if (!BaseConfigurationValidation::deviceName(device_.name)) {
        device_.name = defaults_.deviceName;
    }
    if (!BaseConfigurationValidation::networkConfiguration(network_)) {
        network_.addressMode = NetworkAddressMode::Dhcp;
        if (!BaseConfigurationValidation::hostname(network_.hostname)) network_.hostname = defaults_.hostname;
        if (!BaseConfigurationValidation::wifiSSID(network_.wifiSSID)) network_.wifiSSID = String();
        if (!BaseConfigurationValidation::wifiPassword(network_.wifiPassword)) network_.wifiPassword = String();
    }

}
void BaseSettingsService::validateTime() {
    if (!BaseConfigurationValidation::timezone(time_.timezone)) {
        time_.timezone = String(defaults_.timezone);
    }
    if (!BaseConfigurationValidation::ntpServer(time_.ntpServer1)) {
        time_.ntpServer1 = String(defaults_.ntpServer1);
    }
    if (!BaseConfigurationValidation::ntpServer(time_.ntpServer2)) {
        time_.ntpServer2 = String(defaults_.ntpServer2);
    }
}
bool BaseSettingsService::setDeviceName(const String& deviceName) {
    if (!BaseConfigurationValidation::deviceName(deviceName)) {
        return false;
    }
    if (!storage_.persistString(KeyDeviceName, deviceName)) {
        return false;
    }
    device_.name = deviceName;
    return true;
}
bool BaseSettingsService::setNetworkConfiguration(
    const NetworkConfiguration& requestedNetwork,
    bool updatePassword) {
    NetworkConfiguration network = requestedNetwork;
    if (!updatePassword) network.wifiPassword = network_.wifiPassword;
    if (!BaseConfigurationValidation::networkConfiguration(network)) return false;

    if (!storage_.persistString(KeyHostname, network.hostname)
        || !storage_.persistString(KeyWifiSSID, network.wifiSSID)
        || (updatePassword && !storage_.persistString(KeyWifiPassword, network.wifiPassword))
        || !storage_.persistUInt(KeyAddressMode, static_cast<uint32_t>(network.addressMode))
        || !storage_.persistString(KeyIpv4Address, network.ipv4Address)
        || !storage_.persistString(KeySubnetMask, network.subnetMask)
        || !storage_.persistString(KeyGateway, network.gateway)
        || !storage_.persistString(KeyDns1, network.dns1)
        || !storage_.persistString(KeyDns2, network.dns2)) {
        return false;
    }
    network_ = network;
    return true;
}
bool BaseSettingsService::setWifiSSID(const String& ssid) {
    if (!BaseConfigurationValidation::wifiSSID(ssid)) {
        return false;
    }
    if (!storage_.persistString(KeyWifiSSID, ssid)) {
        return false;
    }
    network_.wifiSSID = ssid;
    return true;
}
bool BaseSettingsService::setWifiPassword(const String& password) {
    if (!BaseConfigurationValidation::wifiPassword(password)) {
        return false;
    }
    if (!storage_.persistString(KeyWifiPassword, password)) {
        return false;
    }
    network_.wifiPassword = password;
    return true;
}
bool BaseSettingsService::setTimezone(const String& timezone) {
    if (!BaseConfigurationValidation::timezone(timezone)) {
        return false;
    }
    if (!storage_.persistString(KeyTimezone, timezone)) {
        return false;
    }
    time_.timezone = timezone;
    return true;
}
bool BaseSettingsService::setNtpServer1(const String& server) {
    if (!BaseConfigurationValidation::ntpServer(server)) {
        return false;
    }
    if (!storage_.persistString(KeyNtpServer1, server)) {
        return false;
    }
    time_.ntpServer1 = server;
    return true;
}
bool BaseSettingsService::setNtpServer2(const String& server) {
    if (!BaseConfigurationValidation::ntpServer(server)) {
        return false;
    }
    if (!storage_.persistString(KeyNtpServer2, server)) {
        return false;
    }
    time_.ntpServer2 = server;
    return true;
}
bool BaseSettingsService::setLocale(Locale locale) {
    if (locale != Locale::GermanGermany
        && locale != Locale::EnglishUnitedKingdom
        && locale != Locale::EnglishUnitedStates) {
        return false;
    }
    if (!storage_.persistUInt(KeyLocale, static_cast<uint32_t>(locale))) {
        return false;
    }
    locale_.locale = locale;
    return true;
}
}
