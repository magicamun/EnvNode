#include "WiFiService.h"
#include <Arduino.h>

namespace WeatherStation {

WiFiService::WiFiService(ILogger& logger, IConfigurationService& configurationService)
    : logger_(logger)
    , configurationService_(configurationService) {
}

void WiFiService::begin() {
    if (state_ != State::Uninitialized) {
        return;
    }

    if (!configurationIsValid()) {
        startSetupAccessPoint();
        return;
    }

    logStateTransition(State::Connecting);
    startConnection();
}

void WiFiService::loop() {
    switch (state_) {
        case State::Connecting:
            updateConnectedState();
            break;
        case State::Connected:
            if (WiFi.status() != WL_CONNECTED) {
                logStateTransition(State::Reconnecting);
                lastReconnectAttemptMs_ = millis();
                WiFi.disconnect();
            }
            break;
        case State::Reconnecting:
            updateReconnectingState();
            break;
        case State::SetupAccessPoint:
        case State::Uninitialized:
            break;
    }
}

bool WiFiService::connected() const {
    return state_ == State::Connected && WiFi.status() == WL_CONNECTED;
}

bool WiFiService::inSetupAccessPointMode() const {
    return state_ == State::SetupAccessPoint;
}

String WiFiService::ipAddress() const {
    if (connected()) {
        return WiFi.localIP().toString();
    }
    return String();
}

void WiFiService::startConnection() {
    const Configuration& configuration = configurationService_.getConfiguration();


    logger_.println("WiFi connection parameters:");
    logger_.printf("  SSID: '%s'\n", configuration.wifiSSID.c_str());
    logger_.printf("  SSID length: %u\n", configuration.wifiSSID.length());
    logger_.printf("  Password length: %u\n", configuration.wifiPassword.length());
    logger_.printf("  Hostname: '%s'\n", configuration.deviceName.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(configuration.deviceName.c_str());

    WiFi.begin(configuration.wifiSSID.c_str(), configuration.wifiPassword.c_str());
    connectionStartTimeMs_ = millis();
    state_ = State::Connecting;
}

void WiFiService::startSetupAccessPoint() {
    const Configuration& configuration = configurationService_.getConfiguration();
    const String apSsid = configuration.deviceName + "-Setup";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str());
    logger_.printf("Setup access point started: %s\n", apSsid.c_str());
    state_ = State::SetupAccessPoint;
}

bool WiFiService::configurationIsValid() const {
    const Configuration& config = configurationService_.getConfiguration();
    return !config.wifiSSID.isEmpty();
}

void WiFiService::logStateTransition(State nextState) {
    if (nextState == state_) {
        return;
    }

    switch (nextState) {
        case State::Connecting:
            logger_.println("WiFi connecting");
            break;
        case State::Connected:
            logger_.println("WiFi connected");
            logger_.printf("IP: %s\n",
               WiFi.localIP().toString().c_str());
            break;
        case State::Reconnecting:
            logger_.println("WiFi reconnecting");
            break;
        case State::SetupAccessPoint:
            logger_.println("setup access point started");
            break;
        case State::Uninitialized:
            break;
    }

    state_ = nextState;
}

void WiFiService::updateConnectedState() {
    if (WiFi.status() == WL_CONNECTED) {
        logStateTransition(State::Connected);
        return;
    }

    if (millis() - connectionStartTimeMs_ >= ConnectionTimeoutMs) {
        startSetupAccessPoint();
    }
}

void WiFiService::updateReconnectingState() {
    if (WiFi.status() == WL_CONNECTED) {
        logStateTransition(State::Connected);
        return;
    }

    if (millis() - lastReconnectAttemptMs_ >= ReconnectIntervalMs) {
        const Configuration& configuration = configurationService_.getConfiguration();
        if (!configuration.wifiSSID.isEmpty()) {
            WiFi.begin(configuration.wifiSSID.c_str(), configuration.wifiPassword.c_str());
        }
        lastReconnectAttemptMs_ = millis();
    }
}

} // namespace WeatherStation
