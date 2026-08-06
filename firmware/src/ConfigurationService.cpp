#include "ConfigurationService.h"
#include <Arduino.h>
#include <Preferences.h>

namespace WeatherStation {

namespace {
constexpr const char* PreferencesNamespace = "weather";
constexpr const char* KeyDeviceName = "deviceName";
constexpr const char* KeyWifiSSID = "wifiSSID";
constexpr const char* KeyWifiPassword = "wifiPassword";
constexpr const char* KeyMqttServer = "mqttServer";
constexpr const char* KeyMqttPort = "mqttPort";
constexpr const char* KeyMqttUsername = "mqttUsername";
constexpr const char* KeyMqttPassword = "mqttPassword";
constexpr uint16_t DefaultMqttPort = 1883;
constexpr size_t MaxDeviceNameLength = 32;
constexpr size_t MaxWifiSSIDLength = 32;
constexpr size_t MaxWifiPasswordLength = 64;
constexpr size_t MaxMqttServerLength = 64;
constexpr size_t MaxMqttUsernameLength = 32;
constexpr size_t MaxMqttPasswordLength = 64;
}

void ConfigurationService::ensurePreferencesStarted() {
    if (!preferencesInitialized_) {
        preferences_.begin(PreferencesNamespace, false);
        preferencesInitialized_ = true;
    }
}

void ConfigurationService::initializeDefaults() {
    configuration_.deviceName = "WeatherStation";
    configuration_.wifiSSID = String();
    configuration_.wifiPassword = String();
    configuration_.mqttServer = String();
    configuration_.mqttPort = DefaultMqttPort;
    configuration_.mqttUsername = String();
    configuration_.mqttPassword = String();
}

void ConfigurationService::loadFromPreferences() {
    ensurePreferencesStarted();

    if (preferences_.isKey(KeyDeviceName)) {
        configuration_.deviceName = preferences_.getString(KeyDeviceName, configuration_.deviceName);
    }
    if (preferences_.isKey(KeyWifiSSID)) {
        configuration_.wifiSSID = preferences_.getString(KeyWifiSSID, configuration_.wifiSSID);
    }
    if (preferences_.isKey(KeyWifiPassword)) {
        configuration_.wifiPassword = preferences_.getString(KeyWifiPassword, configuration_.wifiPassword);
    }
    if (preferences_.isKey(KeyMqttServer)) {
        configuration_.mqttServer = preferences_.getString(KeyMqttServer, configuration_.mqttServer);
    }
    if (preferences_.isKey(KeyMqttPort)) {
        configuration_.mqttPort = static_cast<uint16_t>(preferences_.getUInt(KeyMqttPort, configuration_.mqttPort));
    }
    if (preferences_.isKey(KeyMqttUsername)) {
        configuration_.mqttUsername = preferences_.getString(KeyMqttUsername, configuration_.mqttUsername);
    }
    if (preferences_.isKey(KeyMqttPassword)) {
        configuration_.mqttPassword = preferences_.getString(KeyMqttPassword, configuration_.mqttPassword);
    }
}

void ConfigurationService::validateConfiguration() {
    if (!validateDeviceName(configuration_.deviceName)) {
        configuration_.deviceName = "WeatherStation";
    }
    if (!validateWifiSSID(configuration_.wifiSSID)) {
        configuration_.wifiSSID = String();
    }
    if (!validateWifiPassword(configuration_.wifiPassword)) {
        configuration_.wifiPassword = String();
    }
    if (!validateMqttServer(configuration_.mqttServer)) {
        configuration_.mqttServer = String();
    }
    if (!validateMqttPort(configuration_.mqttPort)) {
        configuration_.mqttPort = DefaultMqttPort;
    }
    if (!validateMqttUsername(configuration_.mqttUsername)) {
        configuration_.mqttUsername = String();
    }
    if (!validateMqttPassword(configuration_.mqttPassword)) {
        configuration_.mqttPassword = String();
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

bool ConfigurationService::setDeviceName(const String& deviceName) {
    if (!validateDeviceName(deviceName)) {
        return false;
    }
    if (!persistString(KeyDeviceName, deviceName)) {
        return false;
    }
    configuration_.deviceName = deviceName;
    return true;
}

bool ConfigurationService::setWifiSSID(const String& ssid) {
    if (!validateWifiSSID(ssid)) {
        return false;
    }
    if (!persistString(KeyWifiSSID, ssid)) {
        return false;
    }
    configuration_.wifiSSID = ssid;
    return true;
}

bool ConfigurationService::setWifiPassword(const String& password) {
    if (!validateWifiPassword(password)) {
        return false;
    }
    if (!persistString(KeyWifiPassword, password)) {
        return false;
    }
    configuration_.wifiPassword = password;
    return true;
}

bool ConfigurationService::setMqttServer(const String& server) {
    if (!validateMqttServer(server)) {
        return false;
    }
    if (!persistString(KeyMqttServer, server)) {
        return false;
    }
    configuration_.mqttServer = server;
    return true;
}

bool ConfigurationService::setMqttPort(uint16_t port) {
    if (!validateMqttPort(port)) {
        return false;
    }
    if (!persistUInt(KeyMqttPort, port)) {
        return false;
    }
    configuration_.mqttPort = port;
    return true;
}

bool ConfigurationService::setMqttUsername(const String& username) {
    if (!validateMqttUsername(username)) {
        return false;
    }
    if (!persistString(KeyMqttUsername, username)) {
        return false;
    }
    configuration_.mqttUsername = username;
    return true;
}

bool ConfigurationService::setMqttPassword(const String& password) {
    if (!validateMqttPassword(password)) {
        return false;
    }
    if (!persistString(KeyMqttPassword, password)) {
        return false;
    }
    configuration_.mqttPassword = password;
    return true;
}

} // namespace WeatherStation
