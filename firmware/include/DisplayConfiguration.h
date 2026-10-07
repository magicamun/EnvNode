#pragma once
#include <Arduino.h>
#include "PropertyTextFormatter.h"
#include "TextDisplayConfiguration.h"

namespace EnvNode {
constexpr size_t DisplayLineCount = 6;
struct DisplayConfiguration {
    bool configured = false;
    TextDisplayConfiguration hardware;
    String formats[DisplayLineCount];
    String sources[DisplayLineCount][MaxPropertySourcesPerLine];
};
// Structural validation does not require live devices or values.
bool validateDisplayConfiguration(const DisplayConfiguration& configuration);
bool encodeDisplayConfiguration(const DisplayConfiguration& configuration, String& encoded);
bool decodeDisplayConfiguration(const String& encoded, DisplayConfiguration& configuration);
}
