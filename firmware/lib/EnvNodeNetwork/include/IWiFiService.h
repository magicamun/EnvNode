#pragma once

#include <Arduino.h>

namespace EnvNode {

// Active station values; empty fields mean no assigned value.
struct NetworkAddressSnapshot {
    String ipAddress;
    String subnetMask;
    String gateway;
    String dns1;
    String dns2;
};

class IWiFiService {
public:
    virtual ~IWiFiService() = default;

    virtual void begin() = 0;
    virtual void loop() = 0;
    virtual bool connected() const = 0;
    virtual bool inSetupAccessPointMode() const = 0;
    virtual String ipAddress() const = 0;
    virtual String hostname() const = 0;
    virtual int32_t rssi() const = 0;
    virtual NetworkAddressSnapshot stationAddresses() const {
        NetworkAddressSnapshot result;
        if (connected()) result.ipAddress = ipAddress();
        return result;
    }
};

} // namespace EnvNode
