#include "ValueMqttTopic.h"
#include "MqttTopic.h"
#include <cstring>
namespace EnvNode {
String mqttValueCommandSubscription(const String& name) { return mqttDeviceTopicRoot(name) + "/value/+/cmd/state"; }
String mqttValueCommandTopic(const String& name, ValueId id) { return mqttDeviceTopicRoot(name) + "/value/" + String(id) + "/cmd/state"; }
String mqttValueStatusTopic(const String& name, ValueId id) { return mqttDeviceTopicRoot(name) + "/value/" + String(id) + "/status/state"; }
String mqttValueDescriptionTopic(const String& name, ValueId id) { return mqttDeviceTopicRoot(name) + "/value/" + String(id) + "/description"; }
bool parseMqttValueCommandTopic(const char* topic, const String& name, ValueId& id) {
    id = 0;
    if (!topic) return false;
    const String prefix = mqttDeviceTopicRoot(name) + "/value/";
    if (strncmp(topic, prefix.c_str(), prefix.length()) != 0) return false;
    const char* p = topic + prefix.length();
    if (*p < '1' || *p > '9') return false;
    uint32_t value = 0;
    while (*p >= '0' && *p <= '9') {
        value = value * 10 + *p++ - '0';
        if (value > 65535) return false;
    }
    if (strcmp(p, "/cmd/state") != 0) return false;
    id = static_cast<ValueId>(value);
    return true;
}
}
