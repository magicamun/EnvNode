#pragma once

#include <Arduino.h>

namespace EnvNode {

class IMqttMessageHandler {
public:
    virtual ~IMqttMessageHandler() = default;
    virtual void handleMqttMessage(
        const char* topic,
        const uint8_t* payload,
        size_t length) = 0;
};

class IMqttService {
public:
    virtual ~IMqttService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool connected() const = 0;
    virtual bool publish(const char* topic, const char* payload, bool retained) = 0;
    virtual bool subscribe(const char* topic) = 0;
    virtual void setMessageHandler(IMqttMessageHandler* handler) = 0;
};

} // namespace EnvNode
