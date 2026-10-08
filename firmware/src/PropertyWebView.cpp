#include "PropertyWebView.h"

#include <cmath>
#include <cstdio>
#include "ElapsedTimeFormatter.h"
#include "HtmlEscaping.h"
#include "UnitConverter.h"

namespace EnvNode {
namespace {

String propertyValue(const PropertyDescription& description, const PropertySnapshot& snapshot) {
    if (!snapshot.valid || snapshot.value.kind() != description.valueKind) return "Invalid";
    const PropertyEnumOption* option = nullptr;
    if (snapshot.value.tryGetEnumeration(option)) {
        return String(option->displayText) + " (" + option->stableCode + ")";
    }
    const char* text = nullptr;
    if (snapshot.value.tryGetText(text)) return String(text);
    char buffer[64];
    float number = 0;
    bool flag = false;
    uint32_t integer = 0;
    if (snapshot.value.tryGetFloatingPoint(number) && std::isfinite(number)) {
        snprintf(buffer, sizeof(buffer), "%.2f", static_cast<double>(number));
    } else if (snapshot.value.tryGetBoolean(flag)) {
        return flag ? description.trueText : description.falseText;
    } else if (snapshot.value.tryGetUnsignedInteger(integer)) {
        snprintf(buffer, sizeof(buffer), "%lu", static_cast<unsigned long>(integer));
    } else {
        return "Invalid";
    }
    String result(buffer);
    const char* unit = UnitConverter::symbol(description.canonicalUnit);
    if (unit != nullptr && unit[0] != '\0') {
        result += " ";
        result += unit;
    }
    return result;
}

} // namespace

String buildPropertyDiagnosticHtml(const IPropertyReader& reader,
    const PropertyReference& reference, uint32_t nowMs) {
    String html("<section class='card property-diagnostic'><h2>Property diagnostic</h2>");
    html += "<p><code>";
    html += reference.componentKind == PropertyComponentKind::Sensor ? "sensor"
        : reference.componentKind == PropertyComponentKind::Actuator ? "actuator"
        : reference.componentKind == PropertyComponentKind::Controller ? "controller"
        : reference.componentKind == PropertyComponentKind::System ? "system"
        : reference.componentKind == PropertyComponentKind::Value ? "value" : "unknown";
    html += " / ";
    html += String(static_cast<unsigned int>(reference.componentId)).c_str();
    html += " / ";
    html += escapeHtml(String(reference.propertyKey)).c_str();
    html += "</code></p>";
    PropertyDescription description;
    PropertySnapshot snapshot;
    const bool described = reader.describe(reference, description);
    const PropertyReadResult status = described
        ? reader.read(reference, snapshot) : PropertyReadResult::UnknownReference;
    if (status == PropertyReadResult::UnknownReference) {
        html += "<p>Unknown reference</p>";
    } else if (status == PropertyReadResult::NoValue) {
        html += reference.componentKind == PropertyComponentKind::Sensor
            ? "<p>No measurement available yet</p>" : "<p>State unavailable</p>";
    } else {
        html += "<dl><dt>Value (canonical unit)</dt><dd>";
        html += escapeHtml(propertyValue(description, snapshot)).c_str();
        html += "</dd><dt>Validity</dt><dd>";
        html += snapshot.valid ? "Valid" : "Invalid";
        html += "</dd><dt>Quality</dt><dd>";
        html += snapshot.hasQuality
            ? escapeHtml(String(measurementQualityDisplayName(snapshot.quality))).c_str()
            : "Not available";
        html += "</dd><dt>Age</dt><dd>";
        html += snapshot.hasAcceptedMonotonicMs
            ? formatElapsedDuration(nowMs - snapshot.acceptedMonotonicMs).c_str()
            : "Not available";
        html += "</dd></dl>";
    }
    if (reference.componentKind == PropertyComponentKind::Controller) {
        html += "<p class='help'>Input evaluation reason; output command success is reported separately by the Controller.</p>";
    }
    if (reference.componentKind == PropertyComponentKind::Actuator) {
        html += "<p class='help'>Logical output command; no mechanical feedback. Change time is not recorded.</p>";
    }
    html += "<p class='help'>Read through the local Property interface. Canonical units; numbers use a decimal point. Refresh to update.</p>";
    html += "<a class='button' href='/measurements'>Refresh</a></section>";
    return html;
}

} // namespace EnvNode
