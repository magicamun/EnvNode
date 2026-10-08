#include "DisplayConfiguration.h"
#include "PropertySourceInput.h"
#include <cstring>
#include <string>

namespace EnvNode {
namespace {
constexpr size_t MaximumEncodedLength = MaxDisplayEncodedLength;
}

const char* textDisplayTypeName(TextDisplayType type) {
    switch (type) {
    case TextDisplayType::Ssd1309: return "SSD1309 · 128×64";
    case TextDisplayType::Ssd1306: return "SSD1306 · 128×64";
    case TextDisplayType::Sh1106: return "SH1106 · 128×64";
    default: return nullptr;
    }
}
bool parseTextDisplayType(const String& value, TextDisplayType& result) {
    if (value == "0") result = TextDisplayType::Ssd1309;
    else if (value == "1") result = TextDisplayType::Ssd1306;
    else if (value == "2") result = TextDisplayType::Sh1106;
    else return false;
    return true;
}

bool validateTextDisplayConfiguration(const TextDisplayConfiguration& configuration) {
    return textDisplayTypeName(configuration.type) != nullptr
        && BoardCapabilities::current().i2cBus(configuration.bus) != nullptr
        && (configuration.address == 0x3C || configuration.address == 0x3D);
}

bool parseTextDisplayConfiguration(const String& enabled, const String& bus, const String& address,
    TextDisplayConfiguration& result) {
    if ((enabled != "0" && enabled != "1") || (bus != "0" && bus != "1")
        || (address != "60" && address != "61")) return false;
    TextDisplayConfiguration candidate = result;
    candidate.enabled = enabled == "1";
    candidate.bus = bus == "0" ? I2CBus::I2C0 : I2CBus::I2C1;
    candidate.address = address == "60" ? 0x3C : 0x3D;
    if (!validateTextDisplayConfiguration(candidate)) return false;
    result = candidate;
    return true;
}

bool validateDisplayBooleanLabel(const String& text) {
    if (text.length() > MaxDisplayBooleanLabelLength || strlen(text.c_str()) != text.length()) return false;
    for (size_t i = 0; i < text.length(); ++i) {
        const unsigned char character = text[i];
        if (character < 32 || character == 127) return false;
    }
    return true;
}

const char* displayEnumText(const DisplayConfiguration& page, size_t line, size_t source, const char* code) {
    for (const auto& entry : page.enumTranslations) {
        if (entry.line == line && entry.source == source && entry.code == code) return entry.text.c_str();
    }
    return "";
}

bool validateDisplayConfiguration(const DisplayConfiguration& configuration) {
    if (configuration.enumTranslations.size() > MaxDisplayEnumTranslations) return false;
    for (size_t i = 0; i < configuration.enumTranslations.size(); ++i) {
        const auto& entry = configuration.enumTranslations[i];
        if (entry.line >= DisplayLineCount || entry.source >= MaxPropertySourcesPerLine
            || configuration.sources[entry.line][entry.source].isEmpty()
            || entry.code.isEmpty() || entry.code.length() > 32
            || entry.text.isEmpty() || !validateDisplayBooleanLabel(entry.text)) return false;
        for (size_t k = 0; k < entry.code.length(); ++k) {
            const char c = entry.code[k];
            if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) return false;
        }
        for (size_t j = 0; j < i; ++j) {
            const auto& previous = configuration.enumTranslations[j];
            if (previous.line == entry.line && previous.source == entry.source && previous.code == entry.code) return false;
        }
    }
    if (!validateTextDisplayConfiguration(configuration.hardware)) return false;
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        const String& format = configuration.formats[line];
        if (format.length() > MaxPropertyFormatLength || strlen(format.c_str()) != format.length()) return false;
        size_t count = 0;
        bool gap = false;
        for (size_t index = 0; index < MaxPropertySourcesPerLine; ++index) {
            const String& source = configuration.sources[line][index];
            const auto& labels = configuration.labels[line][index];
            if (!validateDisplayBooleanLabel(labels.trueText) || !validateDisplayBooleanLabel(labels.falseText)) return false;
            if (source.isEmpty()) {
                if (!labels.trueText.isEmpty() || !labels.falseText.isEmpty()) return false;
                gap = true;
                continue;
            }
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
    // Version 5 adds the driver type; older records default to SSD1309.
    String result("5\n");
    result += String(static_cast<unsigned int>(configuration.hardware.type)).c_str(); result += '\n';
    result += configuration.hardware.enabled ? "1\n" : "0\n";
    result += configuration.hardware.bus == I2CBus::I2C0 ? "0\n" : "1\n";
    result += configuration.hardware.address == 0x3C ? "60\n" : "61\n";
    for (size_t line = 0; line < DisplayLineCount; ++line) {
        result += configuration.formats[line].c_str();
        result += '\n';
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            result += configuration.sources[line][source].c_str(); result += '\n';
            result += configuration.labels[line][source].trueText.c_str(); result += '\n';
            result += configuration.labels[line][source].falseText.c_str(); result += '\n';
        }
    }
    result += String(static_cast<unsigned int>(configuration.enumTranslations.size())).c_str(); result += '\n';
    for (const auto& entry : configuration.enumTranslations) {
        result += String(static_cast<unsigned int>(entry.line)).c_str(); result += '\n';
        result += String(static_cast<unsigned int>(entry.source)).c_str(); result += '\n';
        result += entry.code.c_str(); result += '\n';
        result += entry.text.c_str(); result += '\n';
    }
    if (result.length() > MaximumEncodedLength) return false;
    encoded = result;
    return true;
}

bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration) {
    if (encoded.length() > MaximumEncodedLength || strlen(encoded.c_str()) != encoded.length()) return false;
    const std::string input(encoded.c_str());
    const bool legacy = input.compare(0, 2, "1\n") == 0;
    const bool typed = input.compare(0, 2, "5\n") == 0;
    const bool enums = typed || input.compare(0, 2, "4\n") == 0;
    const bool translated = enums || input.compare(0, 2, "3\n") == 0;
    if (!legacy && !translated && input.compare(0, 2, "2\n") != 0) return false;
    size_t position = 2;
    DisplayConfiguration candidate;
    const auto field = [&](String& target) -> bool {
        const size_t end = input.find('\n', position);
        if (end == std::string::npos) return false;
        target = input.substr(position, end - position).c_str();
        position = end + 1;
        return true;
    };
    if (typed) {
        String type;
        if (!field(type) || !parseTextDisplayType(type, candidate.hardware.type)) return false;
    }
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
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            if (!field(candidate.sources[line][source])) return false;
            if (translated && (!field(candidate.labels[line][source].trueText)
                || !field(candidate.labels[line][source].falseText))) return false;
        }
    }
    if (enums) {
        const auto number = [&](unsigned maximum, unsigned& value) -> bool {
            String text;
            if (!field(text) || text.isEmpty()) return false;
            value = 0;
            for (size_t i = 0; i < text.length(); ++i) {
                if (text[i] < '0' || text[i] > '9') return false;
                value = value * 10 + text[i] - '0';
                if (value > maximum) return false;
            }
            return true;
        };
        unsigned count = 0;
        if (!number(MaxDisplayEnumTranslations, count)) return false;
        for (unsigned i = 0; i < count; ++i) {
            unsigned line = 0, source = 0;
            DisplayEnumTranslation entry;
            if (!number(DisplayLineCount - 1, line) || !number(MaxPropertySourcesPerLine - 1, source)
                || !field(entry.code) || !field(entry.text)) return false;
            entry.line = line; entry.source = source;
            candidate.enumTranslations.push_back(entry);
        }
    }
    if (position != input.size() || !validateDisplayConfiguration(candidate)) return false;
    candidate.configured = true;
    configuration = candidate;
    return true;
}
}
