#include "ValueMqttAdapter.h"
#include "ValueMqttTopic.h"
#include <cstring>
namespace EnvNode {
void ValueMqttAdapter::loop() {
    if (!mqtt_.connected()) { subscription_ = String(); failureReported_ = false; return; }
    const String topic = mqttValueCommandSubscription(configuration_.getConfiguration().device.name);
    if (subscription_ == topic) return;
    if (mqtt_.subscribe(topic.c_str())) {
        subscription_ = topic;
        failureReported_ = false;
    } else if (!failureReported_) {
        logger_.warn("MQTT Value subscription failed; retry pending");
        failureReported_ = true;
    }
}
void ValueMqttAdapter::handleMqttMessage(const char* topic, const uint8_t* payload, size_t length) {
    ValueId id;
    if (!parseMqttValueCommandTopic(topic, configuration_.getConfiguration().device.name, id)) return;
    if (!payload || length == 0 || length > 32) {
        logger_.warn("MQTT Value command rejected: expected option code (1..32 bytes)"); return;
    }
    for (size_t i = 0; i < length; ++i) {
        const auto c = payload[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
            logger_.warn("MQTT Value command rejected: invalid option code"); return;
        }
    }
    char code[33] = {}; memcpy(code, payload, length);
    const auto result = values_.set(id, String(code));
    if (result == ValueCommandResult::Changed || result == ValueCommandResult::Unchanged) {
        logger_.infof("MQTT Value %u accepted: %s", static_cast<unsigned int>(id), code);
    } else {
        logger_.warnf("MQTT Value %u rejected: %s", static_cast<unsigned int>(id),
            result == ValueCommandResult::StorageFailed ? "storage failure; state retained" : "unknown Value or option");
    }
}
}
