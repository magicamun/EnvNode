#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "IWiFiService.h"
#include "IConfigurationService.h"
#include "Logger.h"

namespace WeatherStation {

class WiFiService : public IWiFiService {
public:
    WiFiService(ILogger& logger, IConfigurationService& configurationService);

    void begin() override;
    void loop() override;
    bool connected() const override;
    bool inSetupAccessPointMode() const override;
    String ipAddress() const override;
    String hostname() const override;
    int32_t rssi() const override;

private:
    enum class State {
        Uninitialized,
        Connecting,
        Connected,
        Reconnecting,
        SetupAccessPoint,
    };

    void startConnection();
    bool applyAddressConfiguration(const NetworkConfiguration& network);
    void startSetupAccessPoint();
    bool configurationIsValid() const;
    void logStateTransition(State nextState);
    void updateConnectedState();
    void updateReconnectingState();

    ILogger& logger_;
    IConfigurationService& configurationService_;
    State state_ = State::Uninitialized;
    unsigned long connectionStartTimeMs_ = 0;
    unsigned long lastReconnectAttemptMs_ = 0;
    static constexpr unsigned long ConnectionTimeoutMs = 30000;
    static constexpr unsigned long ReconnectIntervalMs = 5000;
};

} // namespace WeatherStation
