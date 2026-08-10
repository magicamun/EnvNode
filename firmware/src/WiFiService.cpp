#include "WiFiService.h"
#include <Arduino.h>

namespace EnvNode {

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
    if (inSetupAccessPointMode()) {
        return WiFi.softAPIP().toString();
    }
    return String();
}

String WiFiService::hostname() const {
    return configurationService_.getConfiguration().network.hostname;
}

int32_t WiFiService::rssi() const {
    return connected() ? WiFi.RSSI() : 0;
}

void WiFiService::startConnection() {
    const Configuration& configuration = configurationService_.getConfiguration();


    logger_.println("WiFi connection parameters:");
    const NetworkConfiguration& network = configuration.network;
    logger_.printf("  SSID: '%s'\n", network.wifiSSID.c_str());
    logger_.printf("  SSID length: %u\n", network.wifiSSID.length());
    logger_.printf("  Password length: %u\n", network.wifiPassword.length());
    logger_.printf("  Hostname: '%s'\n", network.hostname.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(network.hostname.c_str());
    if (!applyAddressConfiguration(network)) {
        logger_.println("Static network configuration could not be applied");
        startSetupAccessPoint();
        return;
    }

    WiFi.begin(network.wifiSSID.c_str(), network.wifiPassword.c_str());
    connectionStartTimeMs_ = millis();
    state_ = State::Connecting;
}

void WiFiService::startSetupAccessPoint() {
    const Configuration& configuration = configurationService_.getConfiguration();
    const String apSsid = configuration.network.hostname + "-Setup";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSsid.c_str());
    logger_.printf("Setup access point started: %s\n", apSsid.c_str());
    state_ = State::SetupAccessPoint;
}

bool WiFiService::configurationIsValid() const {
    const Configuration& config = configurationService_.getConfiguration();
    return !config.network.wifiSSID.isEmpty();
}

bool WiFiService::applyAddressConfiguration(const NetworkConfiguration& network) {
    if (network.addressMode == NetworkAddressMode::Dhcp) return true;
    IPAddress address, gateway, subnet, dns1, dns2;
    if (!address.fromString(network.ipv4Address)
        || !gateway.fromString(network.gateway)
        || !subnet.fromString(network.subnetMask)
        || !dns1.fromString(network.dns1)) {
        return false;
    }
    if (!network.dns2.isEmpty() && !dns2.fromString(network.dns2)) return false;
    return WiFi.config(address, gateway, subnet, dns1, dns2);
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
        if (!configuration.network.wifiSSID.isEmpty()) {
            WiFi.begin(configuration.network.wifiSSID.c_str(), configuration.network.wifiPassword.c_str());
        }
        lastReconnectAttemptMs_ = millis();
    }
}

} // namespace EnvNode
