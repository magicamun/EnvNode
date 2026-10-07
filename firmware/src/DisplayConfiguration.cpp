#include "DisplayConfiguration.h"
#include "PropertySourceInput.h"
#include <cstring>
#include <string>

namespace EnvNode {
namespace {
constexpr size_t MaximumEncodedLength = 12 + DisplayLineCount *
    (MaxPropertyFormatLength + 1 + MaxPropertySourcesPerLine * (MaxPropertySourceLength + 1));
}

bool validateTextDisplayConfiguration(const TextDisplayConfiguration& configuration) {
    return BoardCapabilities::current().i2cBus(configuration.bus) != nullptr
        && (configuration.address == 0x3C || configuration.address == 0x3D);
}

bool parseTextDisplayConfiguration(const String& enabled, const String& bus, const String& address,
    TextDisplayConfiguration& result) {
    if ((enabled != "0" && enabled != "1") || (bus != "0" && bus != "1")
        || (address != "60" && address != "61")) return false;
    TextDisplayConfiguration candidate;
    candidate.enabled = enabled == "1";
    candidate.bus = bus == "0" ? I2CBus::I2C0 : I2CBus::I2C1;
    candidate.address = address == "60" ? 0x3C : 0x3D;
    if (!validateTextDisplayConfiguration(candidate)) return false;
    result = candidate;
    return true;
}

bool validateDisplayConfiguration(const DisplayConfiguration& configuration) {
    if (!validateTextDisplayConfiguration(configuration.hardware)) return false;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const String& format = configuration.formats[line];
        if (format.length() > MaxPropertyFormatLength || strlen(format.c_str()) != format.length()) return false;
        size_t count = 0;
        bool gap = false;
        for (const String& source : configuration.sources[line]) {
            if (source.isEmpty()) { gap = true; continue; }
            PropertySourceInput parsed;
            if (gap || source.length() > MaxPropertySourceLength
                || strlen(source.c_str()) != source.length()
                || !parsePropertySource(source.c_str(), parsed)) return false;
            ++count;
        }
        if (!validatePropertyFormat(format.c_str(), count)) return false;
    }
    return true;
}

bool encodeDisplayConfiguration(const DisplayConfiguration& configuration, String& encoded) {
    if (!validateDisplayConfiguration(configuration)) return false;
    // Version plus exactly 30 newline-delimited fields. Fields cannot contain control characters.
    String result("2\n");
    result += configuration.hardware.enabled ? "1\n" : "0\n";
    result += configuration.hardware.bus == I2CBus::I2C0 ? "0\n" : "1\n";
    result += configuration.hardware.address == 0x3C ? "60\n" : "61\n";
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        result += configuration.formats[line].c_str();
        result += '\n';
        for (const String& source : configuration.sources[line]) {
            result += source.c_str();
            result += '\n';
        }
    }
    encoded = result;
    return true;
}

bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration) {
    if (encoded.length() > MaximumEncodedLength || strlen(encoded.c_str()) != encoded.length()) return false;
    const std::string input(encoded.c_str());
    const bool legacy = input.compare(0, 2, "1\n") == 0;
    if (!legacy && input.compare(0, 2, "2\n") != 0) return false;
    size_t position = 2;
    DisplayConfiguration candidate;
    const auto field = [&](String& target) -> bool {
        const size_t end = input.find('\n', position);
        if (end == std::string::npos) return false;
        target = input.substr(position, end - position).c_str();
        position = end + 1;
        return true;
    };
    if (!legacy) {
        String enabled, bus, address;
        if (!field(enabled) || !field(bus) || !field(address)
            || (enabled != "0" && enabled != "1")
            || (bus != "0" && bus != "1")
            || (address != "60" && address != "61")) return false;
        candidate.hardware.enabled = enabled == "1";
        candidate.hardware.bus = bus == "0" ? I2CBus::I2C0 : I2CBus::I2C1;
        candidate.hardware.address = address == "60" ? 0x3C : 0x3D;
    }
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        if (!field(candidate.formats[line])) return false;
        for (String& source : candidate.sources[line]) if (!field(source)) return false;
    }
    if (position != input.size() || !validateDisplayConfiguration(candidate)) return false;
    candidate.configured = true;
    configuration = candidate;
    return true;
}
}
