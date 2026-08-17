#pragma once

#include <Arduino.h>
#include "IMqttService.h"
#include "IConfigurationService.h"
#include "IWiFiService.h"
#include "Logger.h"

namespace EnvNode {

class MqttService : public IMqttService {
public:
    MqttService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService);

    void begin() override;
    void loop() override;
    bool connected() const override;
    bool publish(const char* topic, const char* payload, bool retained) override;
    bool subscribe(const char* topic) override;
    void setMessageHandler(IMqttMessageHandler* handler) override;

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
    static void receiveMessage(char* topic, uint8_t* payload, unsigned int length);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IWiFiService& wifiService_;
    IMqttMessageHandler* messageHandler_ = nullptr;

    static MqttService* instance_;

    // MQTT client objects allocated in source file
    State state_ = State::Uninitialized;
    unsigned long lastAttemptMs_ = 0;
    static constexpr unsigned long ReconnectIntervalMs = 5000;
};

} // namespace EnvNode
