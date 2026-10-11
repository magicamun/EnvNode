#include "PropertyWebView.h"
#include "WebPresentationText.h"

#include <cmath>
#include <cstdio>
#include "ElapsedTimeFormatter.h"
#include "HtmlEscaping.h"
#include "UnitConverter.h"

namespace EnvNode {
namespace {

String propertyValue(const PropertyDescription& description, const PropertySnapshot& snapshot, Locale locale) {
    if (!snapshot.valid || snapshot.value.kind() != description.valueKind) return webText(locale, TextId::Invalid);
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
        return webText(locale, TextId::Invalid);
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
    const PropertyReference& reference, uint32_t nowMs, Locale locale) {
    String html((String("<section class='card property-diagnostic'><h2>") + webText(locale, TextId::PropertyDiagnostic) + "</h2>"));
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
        html += (String("<p>") + webText(locale, TextId::UnknownReference) + "</p>");
    } else if (status == PropertyReadResult::NoValue) {
        html += reference.componentKind == PropertyComponentKind::Sensor
            ? (String("<p>") + webText(locale, TextId::NoMeasurementAvailableYet) + "</p>") : (String("<p>") + webText(locale, TextId::StateUnavailable) + "</p>");
    } else {
        html += (String("<dl><dt>") + webText(locale, TextId::ValueCanonicalUnit) + "</dt><dd>");
        html += escapeHtml(propertyValue(description, snapshot, locale)).c_str();
        html += (String("</dd><dt>") + webText(locale, TextId::Validity) + "</dt><dd>");
        html += snapshot.valid ? webText(locale, TextId::Valid) : webText(locale, TextId::Invalid);
        html += (String("</dd><dt>") + webText(locale, TextId::Quality) + "</dt><dd>");
        html += snapshot.hasQuality
            ? escapeHtml(String(webQualityName(locale, snapshot.quality))).c_str()
            : webText(locale, TextId::NotAvailable);
        html += (String("</dd><dt>") + webText(locale, TextId::Age) + "</dt><dd>");
        html += snapshot.hasAcceptedMonotonicMs
            ? formatElapsedDuration(nowMs - snapshot.acceptedMonotonicMs).c_str()
            : webText(locale, TextId::NotAvailable);
        html += "</dd></dl>";
    }
    if (reference.componentKind == PropertyComponentKind::Controller) {
        html += (String("<p class='help'>") + webText(locale, TextId::InputEvaluationReasonOutputCommandSuccessIsReported) + "</p>");
    }
    if (reference.componentKind == PropertyComponentKind::Actuator) {
        html += (String("<p class='help'>") + webText(locale, TextId::LogicalOutputCommandNoMechanicalFeedbackChangeTime) + "</p>");
    }
    html += (String("<p class='help'>") + webText(locale, TextId::ReadThroughTheLocalPropertyInterfaceCanonicalUnits) + "</p>");
    html += (String("<a class='button' href='/measurements'>") + webText(locale, TextId::Refresh) + "</a></section>");
    return html;
}

} // namespace EnvNode
