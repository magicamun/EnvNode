// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"

namespace EnvNode {

enum class NetworkAddressMode : uint8_t {
    Dhcp = 0,
    Static = 1,
};

struct DeviceConfiguration {
    String name;
};

struct NetworkConfiguration {
    String hostname;
    String wifiSSID;
    String wifiPassword;
    NetworkAddressMode addressMode = NetworkAddressMode::Dhcp;
    String ipv4Address;
    String subnetMask;
    String gateway;
    String dns1;
    String dns2;
};

struct TimeConfiguration {
    String timezone;
    String ntpServer1;
    String ntpServer2;
};

struct LocaleConfiguration {
    Locale locale = Locale::GermanGermany;
};

} // namespace EnvNode
