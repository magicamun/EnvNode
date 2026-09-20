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
    logger_.infof("WiFi target SSID='%s' length=%u",
        network.wifiSSID.c_str(),
        static_cast<unsigned int>(network.wifiSSID.length()));

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
        logNetworkScanDiagnostics();
        startSetupAccessPoint();
    }
}

void WiFiService::logNetworkScanDiagnostics() {
    const String configuredSsid =
        configurationService_.getConfiguration().network.wifiSSID;
    logger_.warnf(
        "WiFi connection timeout; scanning for target SSID='%s' length=%u",
        configuredSsid.c_str(),
        static_cast<unsigned int>(configuredSsid.length()));

    // Stop the timed-out association attempt before starting a foreground scan.
    // Keep the SDK's stored AP data untouched; EnvNode owns credentials in NVS.
    WiFi.disconnect(false, false);
    const int16_t networkCount = WiFi.scanNetworks(false, true);
    if (networkCount < 0) {
        logger_.errorf("WiFi diagnostic scan failed: result=%d",
            static_cast<int>(networkCount));
        return;
    }

    bool exactMatch = false;
    logger_.infof("WiFi diagnostic scan found %d networks",
        static_cast<int>(networkCount));
    for (int16_t index = 0; index < networkCount; ++index) {
        const String discoveredSsid = WiFi.SSID(index);
        const bool matches = discoveredSsid == configuredSsid;
        exactMatch = exactMatch || matches;
        logger_.infof(
            "WiFi scan SSID='%s' length=%u channel=%d RSSI=%d%s",
            discoveredSsid.c_str(),
            static_cast<unsigned int>(discoveredSsid.length()),
            static_cast<int>(WiFi.channel(index)),
            static_cast<int>(WiFi.RSSI(index)),
            matches ? " target=exact-match" : "");
    }
    WiFi.scanDelete();

    if (!exactMatch) {
        logger_.error(
            "Configured WiFi SSID was not found exactly in the 2.4 GHz scan");
    } else {
        logger_.warn(
            "Configured WiFi SSID is visible; connection failed after discovery");
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
