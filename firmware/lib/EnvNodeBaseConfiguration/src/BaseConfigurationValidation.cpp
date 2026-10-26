// EnvNode: see repository LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#include "BaseConfigurationValidation.h"
#include <IPAddress.h>
#include <cctype>
namespace EnvNode { namespace BaseConfigurationValidation {
namespace {
constexpr size_t MaxDeviceNameLength = 32;
constexpr size_t MaxWifiSSIDLength = 32;
constexpr size_t MaxWifiPasswordLength = 64;
constexpr size_t MaxTimezoneLength = 128;
constexpr size_t MaxNtpServerLength = 64;
constexpr size_t MaxHostnameLength = 63;
}
bool deviceName(const String& deviceName) {
    return !deviceName.isEmpty() && deviceName.length() <= MaxDeviceNameLength;
}

bool wifiSSID(const String& ssid) {
    return ssid.length() <= MaxWifiSSIDLength;
}

bool wifiPassword(const String& password) {
    return password.length() <= MaxWifiPasswordLength;
}

bool timezone(const String& timezone) {
    return timezone.length() <= MaxTimezoneLength;
}

bool ntpServer(const String& server) {
    return server.length() <= MaxNtpServerLength;
}

bool hostname(const String& hostname) {
    if (hostname.isEmpty() || hostname.length() > MaxHostnameLength) return false;
    if (hostname[0] == '-' || hostname[hostname.length() - 1] == '-') return false;
    for (size_t index = 0; index < hostname.length(); ++index) {
        const char character = hostname[index];
        if (!isalnum(character) && character != '-') return false;
    }
    return true;
}

bool ipv4(const String& value, bool allowEmpty) {
    if (allowEmpty && value.isEmpty()) return true;
    IPAddress address;
    return address.fromString(value) && static_cast<uint32_t>(address) != 0;
}

bool networkConfiguration(const NetworkConfiguration& network) {
    if (!hostname(network.hostname)
        || !wifiSSID(network.wifiSSID)
        || !wifiPassword(network.wifiPassword)) {
        return false;
    }
    if (network.addressMode == NetworkAddressMode::Dhcp) return true;
    if (network.addressMode != NetworkAddressMode::Static
        || !ipv4(network.ipv4Address)
        || !ipv4(network.subnetMask)
        || !ipv4(network.gateway)
        || !ipv4(network.dns1)
        || !ipv4(network.dns2, true)) {
        return false;
    }

    IPAddress address, mask, gateway;
    address.fromString(network.ipv4Address);
    mask.fromString(network.subnetMask);
    gateway.fromString(network.gateway);
    bool zeroSeen = false;
    for (uint8_t byteIndex = 0; byteIndex < 4; ++byteIndex) {
        for (int bit = 7; bit >= 0; --bit) {
            const bool set = (mask[byteIndex] & (1U << bit)) != 0;
            if (!set) zeroSeen = true;
            else if (zeroSeen) return false;
        }
        if ((address[byteIndex] & mask[byteIndex]) != (gateway[byteIndex] & mask[byteIndex])) {
            return false;
        }
    }
    return true;
}

}} // namespace EnvNode::BaseConfigurationValidation
