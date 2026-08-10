#include "MqttService.h"
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <Arduino.h>

namespace EnvNode {

static WiFiClient espClient;
static PubSubClient client(espClient);

MqttService::MqttService(ILogger& logger, IConfigurationService& configurationService, IWiFiService& wifiService)
    : logger_(logger)
    , configurationService_(configurationService)
    , wifiService_(wifiService) {
}

void MqttService::begin() {
    if (state_ != State::Uninitialized) return;
    state_ = State::WaitingForWiFi;
    client.setKeepAlive(60);
    if (!client.setBufferSize(16384)) {
        logger_.println("MQTT packet buffer allocation failed");
    }
}

void MqttService::loop() {
    // If not connected to WiFi, wait
    if (!wifiService_.connected()) {
        if (state_ != State::WaitingForWiFi) {
            logStateTransition(State::WaitingForWiFi);
        }
        return;
    }

    // Ensure MQTT server configured
    const Configuration& cfg = configurationService_.getConfiguration();
    if (cfg.mqtt.server.isEmpty()) {
        // nothing to do until configured
        return;
    }

    if (client.connected()) {
        if (state_ != State::Connected) {
            logStateTransition(State::Connected);
        }
        client.loop();
        return;
    }

    // Not connected: try to connect based on state and timing
    if (state_ == State::WaitingForWiFi || state_ == State::Reconnecting || state_ == State::Uninitialized) {
        attemptConnect();
        return;
    }

    if (state_ == State::Connecting) {
        if (client.connect("")) {
            logStateTransition(State::Connected);
            return;
        }
        // fall through to schedule reconnect
        logStateTransition(State::Reconnecting);
        lastAttemptMs_ = millis();
        return;
    }
}

void MqttService::attemptConnect() {
    unsigned long now = millis();
    if (now - lastAttemptMs_ < ReconnectIntervalMs) return;

    const Configuration& cfg = configurationService_.getConfiguration();
    String clientId = String("EnvNode-") + (cfg.device.name.isEmpty() ? "Device" : cfg.device.name);
    clientId.replace(' ', '-');

    client.setServer(cfg.mqtt.server.c_str(), cfg.mqtt.port);
    logStateTransition(State::Connecting);
    logger_.printf("Connecting to MQTT broker %s:%u user=%s client_id=%s password_length=%u\n",
                    cfg.mqtt.server.c_str(), cfg.mqtt.port,
                    cfg.mqtt.username.c_str(), clientId.c_str(),
                    static_cast<unsigned int>(cfg.mqtt.password.length()));
    // PubSubClient::connect is synchronous; keep it bounded and infrequent
    if (client.connect(clientId.c_str(), cfg.mqtt.username.c_str(), cfg.mqtt.password.c_str())) {
        logStateTransition(State::Connected);
    } else {
        int mqttState = client.state();
        logger_.printf("MQTT connect failed: state=%d broker=%s port=%u user=%s client_id=%s password_length=%u\n",
                        mqttState, cfg.mqtt.server.c_str(), cfg.mqtt.port,
                        cfg.mqtt.username.c_str(), clientId.c_str(),
                        static_cast<unsigned int>(cfg.mqtt.password.length()));
        logStateTransition(State::Reconnecting);
        lastAttemptMs_ = now;
    }
}

void MqttService::logStateTransition(State next) {
    if (next == state_) return;
    switch (next) {
        case State::WaitingForWiFi:
            logger_.println("MQTT waiting for WiFi");
            break;
        case State::Connecting:
            logger_.println("MQTT connecting to broker");
            break;
        case State::Connected:
            logger_.println("MQTT connected");
            break;
        case State::Reconnecting:
            logger_.println("MQTT reconnecting");
            break;
        case State::Uninitialized:
            break;
    }
    state_ = next;
}

bool MqttService::connected() const {
    return client.connected();
}

bool MqttService::publish(const char* topic, const char* payload, bool retained) {
    if (!client.connected()) {
        return false;
    }

    return client.publish(topic, payload, retained);
}

} // namespace EnvNode
