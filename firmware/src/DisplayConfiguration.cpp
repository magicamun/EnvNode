#include "DisplayConfiguration.h"
#include "PropertySourceInput.h"
#include <cstring>
#include <string>
#include <memory>

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

const char* displayEnumText(const DisplayPage& page, size_t line, size_t source, const char* code) {
    for (const auto& entry : page.enumTranslations) {
        if (entry.line == line && entry.source == source && entry.code == code) return entry.text.c_str();
    }
    return "";
}

static bool validatePage(const DisplayPage& configuration) {
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

bool validateDisplayConfiguration(const DisplayConfiguration& configuration) {
    if (!validatePage(configuration) || !validatePage(configuration.secondPage)) return false;
    for (size_t i = 0; i < 2; ++i)
        if (!validateTextDisplayConfiguration(configuration.output(i)) || configuration.pageAssignment[i] > 1) return false;
    return !(configuration.hardware.enabled && configuration.secondHardware.enabled
        && configuration.hardware.bus == configuration.secondHardware.bus
        && configuration.hardware.address == configuration.secondHardware.address);
}

static bool encodeSinglePage(const DisplayPage& configuration, const TextDisplayConfiguration& hardware, String& encoded) {
    if (!validatePage(configuration) || !validateTextDisplayConfiguration(hardware)) return false;
    // Version 5 adds the driver type; older records default to SSD1309.
    String result("5\n");
    result += String(static_cast<unsigned int>(hardware.type)).c_str(); result += '\n';
    result += hardware.enabled ? "1\n" : "0\n";
    result += hardware.bus == I2CBus::I2C0 ? "0\n" : "1\n";
    result += hardware.address == 0x3C ? "60\n" : "61\n";
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

bool encodeDisplayConfiguration(const DisplayConfiguration& configuration, String& encoded) {
    if (!validateDisplayConfiguration(configuration)) return false;
    String first, second;
    if (!encodeSinglePage(configuration.page(0), configuration.output(0), first)
        || !encodeSinglePage(configuration.page(1), configuration.output(1), second)) return false;
    String record("6\n");
    record += String(configuration.pageAssignment[0]).c_str(); record += '\n';
    record += String(configuration.pageAssignment[1]).c_str(); record += '\n';
    record += String(static_cast<unsigned>(first.length())).c_str(); record += '\n';
    record += first.c_str(); record += second.c_str();
    if (record.length() > MaximumEncodedLength) return false;
    encoded = record;
    return true;
}

bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration) {
    if (encoded.length() > MaximumEncodedLength || strlen(encoded.c_str()) != encoded.length()) return false;
    const std::string input(encoded.c_str());
    if (input.compare(0, 2, "6\n") == 0) {
        size_t position = 2;
        unsigned fields[3] = {};
        for (unsigned i = 0; i < 3; ++i) {
            size_t end = input.find('\n', position);
            if (end == std::string::npos || end == position || end - position > 5) return false;
            for (; position < end; ++position) {
                if (input[position] < '0' || input[position] > '9') return false;
                fields[i] = fields[i] * 10 + input[position] - '0';
            }
            ++position;
        }
        if (fields[0] > 1 || fields[1] > 1 || fields[2] > input.size() - position) return false;
        const std::string first = input.substr(position, fields[2]);
        const std::string second = input.substr(position + fields[2]);
        // No nested containers: each page uses the bounded version-5 format.
        if (first.compare(0, 2, "5\n") != 0 || second.compare(0, 2, "5\n") != 0) return false;
        std::unique_ptr<DisplayConfiguration> a(new DisplayConfiguration), b(new DisplayConfiguration);
        if (!decodeDisplayConfiguration(first.c_str(), *a) || !decodeDisplayConfiguration(second.c_str(), *b)) return false;
        a->secondPage = b->page(0); a->secondHardware = b->hardware;
        a->pageAssignment[0] = fields[0]; a->pageAssignment[1] = fields[1];
        if (!validateDisplayConfiguration(*a)) return false;
        configuration = *a;
        return true;
    }

    const bool legacy = input.compare(0, 2, "1\n") == 0;
    const bool typed = input.compare(0, 2, "5\n") == 0;
    const bool enums = typed || input.compare(0, 2, "4\n") == 0;
    const bool translated = enums || input.compare(0, 2, "3\n") == 0;
    if (!legacy && !translated && input.compare(0, 2, "2\n") != 0) return false;
    size_t position = 2;
    std::unique_ptr<DisplayConfiguration> storage(new DisplayConfiguration);
    auto& candidate = *storage;
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
