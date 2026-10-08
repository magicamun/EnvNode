#pragma once
#include "EnumValue.h"
namespace EnvNode {
String mqttValueCommandSubscription(const String& deviceName);
String mqttValueCommandTopic(const String& deviceName, ValueId id);
String mqttValueStatusTopic(const String& deviceName, ValueId id);
String mqttValueDescriptionTopic(const String& deviceName, ValueId id);
bool parseMqttValueCommandTopic(const char* topic, const String& deviceName, ValueId& id);
}
