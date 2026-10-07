#pragma once
#include <Arduino.h>
#include <vector>
#include "PropertyTextFormatter.h"
#include "TextDisplayConfiguration.h"

namespace EnvNode {
constexpr size_t DisplayLineCount = 6;
constexpr size_t MaxDisplayBooleanLabelLength = 16; // UTF-8 bytes, keeps the NVS record below 4000 bytes.
struct DisplayBooleanLabels {
    String trueText;
    String falseText;
};
bool validateDisplayBooleanLabel(const String& text);
constexpr size_t MaxDisplayEnumTranslations = 32;
constexpr size_t MaxDisplayEncodedLength = 6144;
struct DisplayEnumTranslation {
    uint8_t line = 0;
    uint8_t source = 0;
    String code;
    String text;
};
struct DisplayConfiguration {
    bool configured = false;
    TextDisplayConfiguration hardware;
    std::vector<DisplayEnumTranslation> enumTranslations;
    String formats[DisplayLineCount];
    String sources[DisplayLineCount][MaxPropertySourcesPerLine];
    DisplayBooleanLabels labels[DisplayLineCount][MaxPropertySourcesPerLine];
};
const char* displayEnumText(const DisplayConfiguration& page, size_t line, size_t source, const char* code);
// Structural validation does not require live devices or values.
bool validateDisplayConfiguration(const DisplayConfiguration& configuration);
bool encodeDisplayConfiguration(const DisplayConfiguration& configuration, String& encoded);
bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration);
}
