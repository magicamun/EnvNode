#pragma once
#include "HardwareResources.h"
#include <Arduino.h>
namespace EnvNode {
struct TextDisplayConfiguration {
    bool enabled = false;
    I2CBus bus = I2CBus::I2C0;
    uint8_t address = 0x3C;
};
inline bool operator==(const TextDisplayConfiguration& a, const TextDisplayConfiguration& b) {
    return a.enabled == b.enabled && a.bus == b.bus && a.address == b.address;
}
bool parseTextDisplayConfiguration(const String& enabled, const String& bus, const String& address, TextDisplayConfiguration& result);
bool validateTextDisplayConfiguration(const TextDisplayConfiguration& configuration);
}
