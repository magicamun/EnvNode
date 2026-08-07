#include "ConfigurationService.h"
#include <Arduino.h>
#include <Preferences.h>
#include <IPAddress.h>

namespace WeatherStation {

namespace {
constexpr const char* PreferencesNamespace = "weather";
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
constexpr const char* KeyMqttServer = "mqttServer";
constexpr const char* KeyMqttPort = "mqttPort";
constexpr const char* KeyMqttUsername = "mqttUsername";
constexpr const char* KeyMqttPassword = "mqttPassword";
constexpr const char* KeyTimezone = "timezone";
constexpr const char* KeyNtpServer1 = "ntpServer1";
constexpr const char* KeyNtpServer2 = "ntpServer2";
constexpr const char* KeyTemperatureUnit = "unitTemp";
constexpr const char* KeyPressureUnit = "unitPressure";
constexpr const char* KeySolarCellTemperatureUnit = "unitSolarTemp";
constexpr const char* KeyRainDetectorLevelUnit = "unitRainLevel";
constexpr uint16_t DefaultMqttPort = 1883;
constexpr const char* DefaultTimezone = "CET-1CEST,M3.5.0/2,M10.5.0/3";
constexpr const char* DefaultNtpServer1 = "pool.ntp.org";
constexpr const char* DefaultNtpServer2 = "time.cloudflare.com";
constexpr size_t MaxDeviceNameLength = 32;
constexpr size_t MaxWifiSSIDLength = 32;
constexpr size_t MaxWifiPasswordLength = 64;
constexpr size_t MaxMqttServerLength = 64;
constexpr size_t MaxMqttUsernameLength = 32;
constexpr size_t MaxMqttPasswordLength = 64;
constexpr size_t MaxTimezoneLength = 128;
constexpr size_t MaxNtpServerLength = 64;
constexpr size_t MaxHostnameLength = 63;
}

void ConfigurationService::ensurePreferencesStarted() {
    if (!preferencesInitialized_) {
        preferences_.begin(PreferencesNamespace, false);
        preferencesInitialized_ = true;
    }
}

void ConfigurationService::initializeDefaults() {
    configuration_.device.name = "WeatherStation";
    configuration_.network.hostname = "WeatherStation";
    configuration_.network.wifiSSID = String();
    configuration_.network.wifiPassword = String();
    configuration_.network.addressMode = NetworkAddressMode::Dhcp;
    configuration_.network.ipv4Address = String();
    configuration_.network.subnetMask = String();
    configuration_.network.gateway = String();
    configuration_.network.dns1 = String();
    configuration_.network.dns2 = String();
    configuration_.mqtt.server = String();
    configuration_.mqtt.port = DefaultMqttPort;
    configuration_.mqtt.username = String();
    configuration_.mqtt.password = String();
    configuration_.time.timezone = String(DefaultTimezone);
    configuration_.time.ntpServer1 = String(DefaultNtpServer1);
    configuration_.time.ntpServer2 = String(DefaultNtpServer2);
    configuration_.presentation.temperature =
        measurementTypeMetadata(MeasurementType::Temperature).defaultPresentationUnit;
    configuration_.presentation.atmosphericPressure =
        measurementTypeMetadata(MeasurementType::AtmosphericPressure).defaultPresentationUnit;
    configuration_.presentation.solarCellTemperature =
        measurementTypeMetadata(MeasurementType::SolarCellTemperature).defaultPresentationUnit;
    configuration_.presentation.rainDetectorLevel =
        measurementTypeMetadata(MeasurementType::RainDetectorLevel).defaultPresentationUnit;
}

void ConfigurationService::loadFromPreferences() {
    ensurePreferencesStarted();

    if (preferences_.isKey(KeyDeviceName)) {
        configuration_.device.name = preferences_.getString(KeyDeviceName, configuration_.device.name);
    }
    if (preferences_.isKey(KeyWifiSSID)) {
        configuration_.network.wifiSSID = preferences_.getString(KeyWifiSSID, configuration_.network.wifiSSID);
    }
    if (preferences_.isKey(KeyWifiPassword)) {
        configuration_.network.wifiPassword = preferences_.getString(KeyWifiPassword, configuration_.network.wifiPassword);
    }
    configuration_.network.hostname = preferences_.getString(KeyHostname, configuration_.device.name);
    const uint32_t addressMode = preferences_.getUInt(KeyAddressMode, 0);
    configuration_.network.addressMode = addressMode == 1
        ? NetworkAddressMode::Static : NetworkAddressMode::Dhcp;
    configuration_.network.ipv4Address = preferences_.getString(KeyIpv4Address, "");
    configuration_.network.subnetMask = preferences_.getString(KeySubnetMask, "");
    configuration_.network.gateway = preferences_.getString(KeyGateway, "");
    configuration_.network.dns1 = preferences_.getString(KeyDns1, "");
    configuration_.network.dns2 = preferences_.getString(KeyDns2, "");
    if (preferences_.isKey(KeyMqttServer)) {
        configuration_.mqtt.server = preferences_.getString(KeyMqttServer, configuration_.mqtt.server);
    }
    if (preferences_.isKey(KeyMqttPort)) {
        configuration_.mqtt.port = static_cast<uint16_t>(preferences_.getUInt(KeyMqttPort, configuration_.mqtt.port));
    }
    if (preferences_.isKey(KeyMqttUsername)) {
        configuration_.mqtt.username = preferences_.getString(KeyMqttUsername, configuration_.mqtt.username);
    }
    if (preferences_.isKey(KeyMqttPassword)) {
        configuration_.mqtt.password = preferences_.getString(KeyMqttPassword, configuration_.mqtt.password);
    }
    if (preferences_.isKey(KeyTimezone)) {
        configuration_.time.timezone = preferences_.getString(KeyTimezone, configuration_.time.timezone);
    }
    if (preferences_.isKey(KeyNtpServer1)) {
        configuration_.time.ntpServer1 = preferences_.getString(KeyNtpServer1, configuration_.time.ntpServer1);
    }
    if (preferences_.isKey(KeyNtpServer2)) {
        configuration_.time.ntpServer2 = preferences_.getString(KeyNtpServer2, configuration_.time.ntpServer2);
    }
    configuration_.presentation.temperature =
        loadPresentationUnit(KeyTemperatureUnit, MeasurementType::Temperature);
    configuration_.presentation.atmosphericPressure =
        loadPresentationUnit(KeyPressureUnit, MeasurementType::AtmosphericPressure);
    configuration_.presentation.solarCellTemperature =
        loadPresentationUnit(KeySolarCellTemperatureUnit, MeasurementType::SolarCellTemperature);
    configuration_.presentation.rainDetectorLevel =
        loadPresentationUnit(KeyRainDetectorLevelUnit, MeasurementType::RainDetectorLevel);
}

PresentationUnit ConfigurationService::loadPresentationUnit(const char* key, MeasurementType type) {
    const PresentationUnit fallback = measurementTypeMetadata(type).defaultPresentationUnit;
    if (!preferences_.isKey(key)) {
        return fallback;
    }

    const uint32_t encoded = preferences_.getUInt(key, static_cast<uint32_t>(fallback));
    if (encoded > static_cast<uint32_t>(PresentationUnit::Ratio)) {
        return fallback;
    }
    const PresentationUnit unit = static_cast<PresentationUnit>(encoded);
    return supportsPresentationUnit(type, unit) ? unit : fallback;
}

void ConfigurationService::validateConfiguration() {
    if (!validateDeviceName(configuration_.device.name)) {
        configuration_.device.name = "WeatherStation";
    }
    if (!validateNetworkConfiguration(configuration_.network)) {
        configuration_.network.addressMode = NetworkAddressMode::Dhcp;
        if (!validateHostname(configuration_.network.hostname)) configuration_.network.hostname = "WeatherStation";
        if (!validateWifiSSID(configuration_.network.wifiSSID)) configuration_.network.wifiSSID = String();
        if (!validateWifiPassword(configuration_.network.wifiPassword)) configuration_.network.wifiPassword = String();
    }
    if (!validateMqttServer(configuration_.mqtt.server)) {
        configuration_.mqtt.server = String();
    }
    if (!validateMqttPort(configuration_.mqtt.port)) {
        configuration_.mqtt.port = DefaultMqttPort;
    }
    if (!validateMqttUsername(configuration_.mqtt.username)) {
        configuration_.mqtt.username = String();
    }
    if (!validateMqttPassword(configuration_.mqtt.password)) {
        configuration_.mqtt.password = String();
    }
    if (!validateTimezone(configuration_.time.timezone)) {
        configuration_.time.timezone = String(DefaultTimezone);
    }
    if (!validateNtpServer(configuration_.time.ntpServer1)) {
        configuration_.time.ntpServer1 = String(DefaultNtpServer1);
    }
    if (!validateNtpServer(configuration_.time.ntpServer2)) {
        configuration_.time.ntpServer2 = String(DefaultNtpServer2);
    }
}

void ConfigurationService::loadConfiguration() {
    initializeDefaults();
    loadFromPreferences();
    validateConfiguration();
}

const Configuration& ConfigurationService::getConfiguration() const {
    return configuration_;
}

bool ConfigurationService::persistString(const char* key, const String& value) {
    ensurePreferencesStarted();
    if (value.isEmpty()) {
        if (!preferences_.isKey(key)) {
            return true;
        }
        return preferences_.remove(key);
    }
    return preferences_.putString(key, value) > 0;
}

bool ConfigurationService::persistUInt(const char* key, uint32_t value) {
    ensurePreferencesStarted();
    return preferences_.putUInt(key, value);
}

bool ConfigurationService::validateDeviceName(const String& deviceName) const {
    return !deviceName.isEmpty() && deviceName.length() <= MaxDeviceNameLength;
}

bool ConfigurationService::validateWifiSSID(const String& ssid) const {
    return ssid.length() <= MaxWifiSSIDLength;
}

bool ConfigurationService::validateWifiPassword(const String& password) const {
    return password.length() <= MaxWifiPasswordLength;
}

bool ConfigurationService::validateMqttServer(const String& server) const {
    if (server.isEmpty()) return true;
    if (server.length() > MaxMqttServerLength) return false;
    // reject whitespace-only strings
    for (size_t i = 0; i < server.length(); ++i) {
        if (!isspace(server[i])) return true;
    }
    return false;
}

bool ConfigurationService::validateMqttPort(uint16_t port) const {
    return port >= 1 && port <= 65535;
}

bool ConfigurationService::validateMqttUsername(const String& username) const {
    return username.length() <= MaxMqttUsernameLength;
}

bool ConfigurationService::validateMqttPassword(const String& password) const {
    return password.length() <= MaxMqttPasswordLength;
}

bool ConfigurationService::validateTimezone(const String& timezone) const {
    return timezone.length() <= MaxTimezoneLength;
}

bool ConfigurationService::validateNtpServer(const String& server) const {
    return server.length() <= MaxNtpServerLength;
}

bool ConfigurationService::validateHostname(const String& hostname) const {
    if (hostname.isEmpty() || hostname.length() > MaxHostnameLength) return false;
    if (hostname[0] == '-' || hostname[hostname.length() - 1] == '-') return false;
    for (size_t index = 0; index < hostname.length(); ++index) {
        const char character = hostname[index];
        if (!isalnum(character) && character != '-') return false;
    }
    return true;
}

bool ConfigurationService::validateIPv4(const String& value, bool allowEmpty) const {
    if (allowEmpty && value.isEmpty()) return true;
    IPAddress address;
    return address.fromString(value) && static_cast<uint32_t>(address) != 0;
}

bool ConfigurationService::validateNetworkConfiguration(const NetworkConfiguration& network) const {
    if (!validateHostname(network.hostname)
        || !validateWifiSSID(network.wifiSSID)
        || !validateWifiPassword(network.wifiPassword)) {
        return false;
    }
    if (network.addressMode == NetworkAddressMode::Dhcp) return true;
    if (network.addressMode != NetworkAddressMode::Static
        || !validateIPv4(network.ipv4Address)
        || !validateIPv4(network.subnetMask)
        || !validateIPv4(network.gateway)
        || !validateIPv4(network.dns1)
        || !validateIPv4(network.dns2, true)) {
        return false;
    }

    IPAddress address, mask, gateway;
    address.fromString(network.ipv4Address);
    mask.fromString(network.subnetMask);
    gateway.fromString(network.gateway);
    bool zeroSeen = false;
    for (uint8_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
        for (int bit = 7; bit >= 0; --bit) {
            const bool set = (mask[byteIndex] & (1U << bit)) != 0;
            if (!set) zeroSeen = true;
            else if (zeroSeen) return false;
        }
        if ((address[byteIndex] & mask[byteIndex]) != (gateway[byteIndex] & mask[byteIndex])) {
            return false;
        }
    }
    return true;
}

bool ConfigurationService::setDeviceName(const String& deviceName) {
    if (!validateDeviceName(deviceName)) {
        return false;
    }
    if (!persistString(KeyDeviceName, deviceName)) {
        return false;
    }
    configuration_.device.name = deviceName;
    return true;
}

bool ConfigurationService::setNetworkConfiguration(
    const NetworkConfiguration& requestedNetwork,
    bool updatePassword) {
    NetworkConfiguration network = requestedNetwork;
    if (!updatePassword) network.wifiPassword = configuration_.network.wifiPassword;
    if (!validateNetworkConfiguration(network)) return false;

    if (!persistString(KeyHostname, network.hostname)
        || !persistString(KeyWifiSSID, network.wifiSSID)
        || (updatePassword && !persistString(KeyWifiPassword, network.wifiPassword))
        || !persistUInt(KeyAddressMode, static_cast<uint32_t>(network.addressMode))
        || !persistString(KeyIpv4Address, network.ipv4Address)
        || !persistString(KeySubnetMask, network.subnetMask)
        || !persistString(KeyGateway, network.gateway)
        || !persistString(KeyDns1, network.dns1)
        || !persistString(KeyDns2, network.dns2)) {
        return false;
    }
    configuration_.network = network;
    return true;
}

bool ConfigurationService::setWifiSSID(const String& ssid) {
    if (!validateWifiSSID(ssid)) {
        return false;
    }
    if (!persistString(KeyWifiSSID, ssid)) {
        return false;
    }
    configuration_.network.wifiSSID = ssid;
    return true;
}

bool ConfigurationService::setWifiPassword(const String& password) {
    if (!validateWifiPassword(password)) {
        return false;
    }
    if (!persistString(KeyWifiPassword, password)) {
        return false;
    }
    configuration_.network.wifiPassword = password;
    return true;
}

bool ConfigurationService::setMqttServer(const String& server) {
    if (!validateMqttServer(server)) {
        return false;
    }
    if (!persistString(KeyMqttServer, server)) {
        return false;
    }
    configuration_.mqtt.server = server;
    return true;
}

bool ConfigurationService::setMqttPort(uint16_t port) {
    if (!validateMqttPort(port)) {
        return false;
    }
    if (!persistUInt(KeyMqttPort, port)) {
        return false;
    }
    configuration_.mqtt.port = port;
    return true;
}

bool ConfigurationService::setMqttUsername(const String& username) {
    if (!validateMqttUsername(username)) {
        return false;
    }
    if (!persistString(KeyMqttUsername, username)) {
        return false;
    }
    configuration_.mqtt.username = username;
    return true;
}

bool ConfigurationService::setMqttPassword(const String& password) {
    if (!validateMqttPassword(password)) {
        return false;
    }
    if (!persistString(KeyMqttPassword, password)) {
        return false;
    }
    configuration_.mqtt.password = password;
    return true;
}

bool ConfigurationService::setTimezone(const String& timezone) {
    if (!validateTimezone(timezone)) {
        return false;
    }
    if (!persistString(KeyTimezone, timezone)) {
        return false;
    }
    configuration_.time.timezone = timezone;
    return true;
}

bool ConfigurationService::setNtpServer1(const String& server) {
    if (!validateNtpServer(server)) {
        return false;
    }
    if (!persistString(KeyNtpServer1, server)) {
        return false;
    }
    configuration_.time.ntpServer1 = server;
    return true;
}

bool ConfigurationService::setNtpServer2(const String& server) {
    if (!validateNtpServer(server)) {
        return false;
    }
    if (!persistString(KeyNtpServer2, server)) {
        return false;
    }
    configuration_.time.ntpServer2 = server;
    return true;
}

bool ConfigurationService::setPresentationUnit(MeasurementType type, PresentationUnit unit) {
    if (!supportsPresentationUnit(type, unit)) {
        return false;
    }

    const char* key = nullptr;
    PresentationUnit* configuredUnit = nullptr;
    switch (type) {
        case MeasurementType::Temperature:
            key = KeyTemperatureUnit;
            configuredUnit = &configuration_.presentation.temperature;
            break;
        case MeasurementType::AtmosphericPressure:
            key = KeyPressureUnit;
            configuredUnit = &configuration_.presentation.atmosphericPressure;
            break;
        case MeasurementType::SolarCellTemperature:
            key = KeySolarCellTemperatureUnit;
            configuredUnit = &configuration_.presentation.solarCellTemperature;
            break;
        case MeasurementType::RainDetectorLevel:
            key = KeyRainDetectorLevelUnit;
            configuredUnit = &configuration_.presentation.rainDetectorLevel;
            break;
        default:
            return false;
    }

    if (!persistUInt(key, static_cast<uint32_t>(unit))) {
        return false;
    }
    *configuredUnit = unit;
    return true;
}

bool ConfigurationService::resetToDefaults() {
    ensurePreferencesStarted();
    if (!preferences_.clear()) {
        return false;
    }
    initializeDefaults();
    return true;
}

} // namespace WeatherStation
