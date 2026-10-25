// EnvNode: see repository LICENSE.
#pragma once
#include <Arduino.h>
#include <functional>
namespace EnvNode {
// UI fragments are supplied by the application; user-visible text is escaped.
struct WebPageFrameData {
    const char* title;
    const String& deviceName;
    const char* productName;
    const char* semanticVersion;
    const char* buildTimestamp;
    const char* languageTag;
    std::function<String()> uiScript;
    std::function<String()> navigation;
    std::function<String()> notices;
    bool setupAccessPoint;
    const char* setupMessage;
    bool wideContent;
};
String buildWebPageHeader(const WebPageFrameData& page);
const char* webPageFooter();
}
