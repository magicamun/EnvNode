// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "BaseConfiguration.h"
namespace EnvNode { namespace BaseConfigurationValidation {
bool deviceName(const String& deviceName);
bool wifiSSID(const String& ssid);
bool wifiPassword(const String& password);
bool timezone(const String& timezone);
bool ntpServer(const String& server);
bool hostname(const String& hostname);
bool ipv4(const String& value, bool allowEmpty = false);
bool networkConfiguration(const NetworkConfiguration& network);
}} // namespace EnvNode::BaseConfigurationValidation
