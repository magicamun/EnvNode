#include "PropertyPreviewWebView.h"

#include <cstring>
#include "HtmlEscaping.h"
#include "PropertyTextFormatter.h"
#include "UnitConverter.h"

namespace EnvNode {

bool parsePropertySource(const char* text, PropertySourceInput& result) {
    result = PropertySourceInput{};
    if (text == nullptr) return false;
    size_t length = 0;
    while (length <= MaxPropertySourceLength && text[length]) ++length;
    if (length > MaxPropertySourceLength) return false;
    const char* slash = strchr(text, '/');
    if (slash == nullptr) return false;
    PropertySourceInput parsed;
    const size_t categoryLength = slash - text;
    if (categoryLength == 6 && strncmp(text, "sensor", 6) == 0) parsed.kind = PropertyComponentKind::Sensor;
    else if (categoryLength == 8 && strncmp(text, "actuator", 8) == 0) parsed.kind = PropertyComponentKind::Actuator;
    else if (categoryLength == 10 && strncmp(text, "controller", 10) == 0) parsed.kind = PropertyComponentKind::Controller;
    else return false;
    const char* current = slash + 1;
    if (*current < '0' || *current > '9') return false;
    uint32_t id = 0;
    while (*current >= '0' && *current <= '9') {
        id = id * 10 + *current++ - '0';
        if (id > UINT16_MAX) return false;
    }
    if (id == 0 || *current++ != '/') return false;
    const size_t keyLength = strlen(current);
    if (keyLength == 0 || keyLength >= sizeof(parsed.key)) return false;
    for (size_t i = 0; i < keyLength; ++i) {
        const char c = current[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '_')) return false;
    }
    parsed.id = static_cast<uint16_t>(id);
    memcpy(parsed.key, current, keyLength + 1);
    result = parsed;
    return true;
}

String propertySourceText(const PropertyReference& reference) {
    const char* category = reference.componentKind == PropertyComponentKind::Sensor ? "sensor"
        : reference.componentKind == PropertyComponentKind::Actuator ? "actuator"
        : reference.componentKind == PropertyComponentKind::Controller ? "controller" : "unknown";
    return String(category) + "/" + String(static_cast<unsigned int>(reference.componentId))
        + "/" + reference.propertyKey;
}

String buildPropertySourceOption(const PropertyReference& reference,
    const PropertyDescription& description, const char* componentName, const String& selectedSource) {
    const String source = propertySourceText(reference);
    String html("<option value='");
    html += escapeHtml(source).c_str();
    html += source == selectedSource ? "' selected>" : "'>";
    html += escapeHtml(source).c_str();
    html += " · ";
    html += escapeHtml(String(componentName)).c_str();
    const char* unit = UnitConverter::symbol(description.canonicalUnit);
    if (unit != nullptr && unit[0]) {
        html += " (";
        html += escapeHtml(String(unit)).c_str();
        html += ")";
    }
    html += "</option>";
    return html;
}

String buildPropertyPreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const String& source, const String& format, bool submitted, bool sourceListed) {
    String html("<section class='card' id='text-preview'><h2>Text line preview</h2>");
    html += "<form method='get' action='/measurements#text-preview'><label for='preview-source'>Source</label>";
    html += "<select id='preview-source' name='source' required>";
    if (!sourceListed) {
        html += "<option value='";
        if (source.length() <= MaxPropertySourceLength) html += escapeHtml(source).c_str();
        html += "' selected>";
        html += source.isEmpty() ? "Choose a source" : "Selected source unavailable";
        html += "</option>";
    }
    html += optionsHtml.c_str();
    html += "</select><label for='preview-format'>Format</label>";
    html += "<input id='preview-format' name='format' maxlength='128' required value='";
    if (format.length() <= MaxPropertyFormatLength) html += escapeHtml(format).c_str();
    html += "'><button type='submit' name='preview' value='1'>Preview / refresh</button></form>";
    html += "<p class='help'>One source and one placeholder: %.1f for decimals, %u for unsigned integers, %s for On/Off or enum text. Width: %8.1f; left aligned: %-8s; zero padded: %08.1f; percent sign: %%. Decimal places: 0–6; width: up to 64.</p>";
    html += "<p class='help'>Numbers use the source's canonical unit and a decimal point. Add unit text to the format. Preview settings stay in this URL; they are not saved on the device. Refresh to read the latest value.</p>";
    if (optionsHtml.isEmpty()) html += "<p>No supported sources are currently available.</p>";
    if (submitted) {
        PropertySourceInput parsed;
        if (!parsePropertySource(source.c_str(), parsed)) {
            html += "<p role='alert'>Choose a valid source (category / ID / property).</p>";
        } else {
            const PropertyTextResult result = formatPropertyText(reader, parsed.reference(), format.c_str());
            if (result.status == PropertyFormatStatus::Formatted) {
                html += "<p>Result</p><pre style='white-space:pre-wrap;overflow-wrap:anywhere'>";
                html += escapeHtml(String(result.text)).c_str();
                html += "</pre>";
            } else {
                html += "<p role='alert'>";
                html += escapeHtml(String(propertyFormatError(result.status))).c_str();
                html += "</p>";
            }
        }
    }
    html += "</section>";
    return html;
}

} // namespace EnvNode
