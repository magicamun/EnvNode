#pragma once

#include <Arduino.h>
#include "IMqttService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "Logger.h"

namespace WeatherStation {

class MqttService : public IMqttService {
public:
    MqttService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService);

    void begin() override;
    void loop() override;
    bool connected() const override;

private:
    enum class State {
        Uninitialized,
        WaitingForWiFi,
        Connecting,
        Connected,
        Reconnecting,
    };

    void attemptConnect();
    void logStateTransition(State next);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;

    // MQTT client objects allocated in source file
    State state_ = State::Uninitialized;
    unsigned long lastAttemptMs_ = 0;
    static constexpr unsigned long ReconnectIntervalMs = 5000;
};

} // namespace WeatherStation
