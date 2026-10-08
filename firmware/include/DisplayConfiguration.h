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
constexpr size_t MaxDisplayEncodedLength = 12400;
struct DisplayEnumTranslation {
    uint8_t line = 0;
    uint8_t source = 0;
    String code;
    String text;
};
struct DisplayPage {
    std::vector<DisplayEnumTranslation> enumTranslations;
    String formats[DisplayLineCount];
    String sources[DisplayLineCount][MaxPropertySourcesPerLine];
    DisplayBooleanLabels labels[DisplayLineCount][MaxPropertySourcesPerLine];
};
// Page 1 retains its field access for existing callers and legacy records.
struct DisplayConfiguration : DisplayPage {
    bool configured = false;
    TextDisplayConfiguration hardware;
    TextDisplayConfiguration secondHardware;
    DisplayPage secondPage;
    uint8_t pageAssignment[2] = {0, 1};
    TextDisplayConfiguration& output(size_t index) { return index == 0 ? hardware : secondHardware; }
    const TextDisplayConfiguration& output(size_t index) const { return index == 0 ? hardware : secondHardware; }
    DisplayPage& page(size_t index) { return index == 0 ? static_cast<DisplayPage&>(*this) : secondPage; }
    const DisplayPage& page(size_t index) const { return index == 0 ? static_cast<const DisplayPage&>(*this) : secondPage; }
};
const char* displayEnumText(const DisplayPage& page, size_t line, size_t source, const char* code);
// Structural validation does not require live devices or values.
bool validateDisplayConfiguration(const DisplayConfiguration& configuration);
bool encodeDisplayConfiguration(const DisplayConfiguration& configuration, String& encoded);
bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration);
}
