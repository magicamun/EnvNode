#pragma once
#include "HardwareResources.h"
#include <Arduino.h>
namespace EnvNode {
enum class TextDisplayType : uint8_t { Ssd1309 = 0, Ssd1306 = 1, Sh1106 = 2 };
const char* textDisplayTypeName(TextDisplayType type);
bool parseTextDisplayType(const String& value, TextDisplayType& result);
struct TextDisplayConfiguration {
    TextDisplayType type = TextDisplayType::Ssd1309;
    bool enabled = false;
    I2CBus bus = I2CBus::I2C0;
    uint8_t address = 0x3C;
};
inline bool operator==(const TextDisplayConfiguration& a, const TextDisplayConfiguration& b) {
    return a.type == b.type && a.enabled == b.enabled && a.bus == b.bus && a.address == b.address;
}
bool parseTextDisplayConfiguration(const String& enabled, const String& bus, const String& address, TextDisplayConfiguration& result);
bool validateTextDisplayConfiguration(const TextDisplayConfiguration& configuration);
}
