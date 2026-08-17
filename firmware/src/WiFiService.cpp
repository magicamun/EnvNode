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


    logger_.debug("WiFi connection parameters:");
    const NetworkConfiguration& network = configuration.network;
    logger_.debugf("SSID: '%s'", network.wifiSSID.c_str());
    logger_.debugf("SSID length: %u", network.wifiSSID.length());
    logger_.debugf("Password length: %u", network.wifiPassword.length());
    logger_.debugf("Hostname: '%s'", network.hostname.c_str());

    WiFi.mode(WIFI_STA);
    WiFi.setHostname(network.hostname.c_str());
    if (!applyAddressConfiguration(network)) {
        logger_.error("Static network configuration could not be applied");
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
    logger_.infof("Setup access point started: %s", apSsid.c_str());
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
            logger_.info("WiFi connecting");
            break;
        case State::Connected:
            logger_.info("WiFi connected");
            logger_.infof("IP: %s",
               WiFi.localIP().toString().c_str());
            break;
        case State::Reconnecting:
            logger_.warn("WiFi reconnecting");
            break;
        case State::SetupAccessPoint:
            logger_.info("Setup access point started");
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
