// EnvNode test double: see repository LICENSE.
#pragma once
#include "Arduino.h"
#include "IPAddress.h"
#include <vector>
constexpr int WIFI_STA = 1;
constexpr int WIFI_AP = 2;
constexpr int WL_CONNECTED = 3;
constexpr int WL_DISCONNECTED = 6;
class WiFiClass {
public:
    struct AccessPoint { String ssid; int32_t rssi; int channel; };
    int currentStatus = WL_DISCONNECTED;
    int currentMode = 0;
    int beginCalls = 0, apCalls = 0, disconnectCalls = 0, configCalls = 0;
    int scanCalls = 0, scanDeleteCalls = 0;
    int16_t scanResult = 0;
    bool configResult = true, lastWifiOff = false, lastErase = false;
    String configuredHostname, connectedSsid, connectedPassword, apSsid;
    IPAddress address, gateway, mask, firstDns, secondDns, apAddress;
    std::vector<AccessPoint> accessPoints;
    int status() const { return currentStatus; }
    void mode(int value) { currentMode = value; }
    bool setHostname(const char* value) { configuredHostname = value; return true; }
    void begin(const char* ssid, const char* password) {
        ++beginCalls; connectedSsid = ssid; connectedPassword = password;
    }
    bool softAP(const char* ssid) { ++apCalls; apSsid = ssid; return true; }
    void disconnect(bool wifiOff = false, bool erase = false) {
        ++disconnectCalls; lastWifiOff = wifiOff; lastErase = erase;
    }
    bool config(IPAddress a, IPAddress g, IPAddress m, IPAddress d1, IPAddress d2) {
        ++configCalls; address = a; gateway = g; mask = m; firstDns = d1; secondDns = d2;
        return configResult;
    }
    IPAddress localIP() const { return address; }
    IPAddress gatewayIP() const { return gateway; }
    IPAddress subnetMask() const { return mask; }
    IPAddress dnsIP(int index) const { return index == 0 ? firstDns : secondDns; }
    IPAddress softAPIP() const { return apAddress; }
    int32_t RSSI() const { return -42; }
    int32_t RSSI(int index) const { return accessPoints[index].rssi; }
    int channel(int index) const { return accessPoints[index].channel; }
    String SSID(int index) const { return accessPoints[index].ssid; }
    int16_t scanNetworks(bool, bool) { ++scanCalls; return scanResult; }
    void scanDelete() { ++scanDeleteCalls; }
};
extern WiFiClass WiFi;
