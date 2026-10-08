#include "PropertyPreviewWebView.h"
#include "DisplayPageFormatter.h"

#include <cstring>
#include "HtmlEscaping.h"
#include "JsonWriter.h"
#include "PropertyTextFormatter.h"
#include "UnitConverter.h"

namespace EnvNode {

String propertySourceText(const PropertyReference& reference) {
    const char* category = reference.componentKind == PropertyComponentKind::Sensor ? "sensor"
        : reference.componentKind == PropertyComponentKind::Actuator ? "actuator"
        : reference.componentKind == PropertyComponentKind::Controller ? "controller"
        : reference.componentKind == PropertyComponentKind::System ? "system"
        : reference.componentKind == PropertyComponentKind::Value ? "value" : "unknown";
    return String(category) + "/" + String(static_cast<unsigned int>(reference.componentId))
        + "/" + reference.propertyKey;
}

String buildPropertySourceOption(const PropertyReference& reference,
    const PropertyDescription& description, const char* componentName, const String& selectedSource) {
    const String source = propertySourceText(reference);
    String html("<option value='");
    html += escapeHtml(source).c_str();
    html += "' data-boolean='";
    html += description.valueKind == PropertyValueKind::Boolean ? "1" : "0";
    html += "' data-enum='";
    String choices("[");
    if (description.valueKind == PropertyValueKind::Enumeration) for (size_t i = 0; i < description.enumOptionCount; ++i) {
        if (i) choices += ',';
        choices += '['; appendJsonString(choices, description.enumOptions[i].stableCode);
        choices += ','; appendJsonString(choices, description.enumOptions[i].displayText); choices += ']';
    }
    choices += ']';
    html += escapeHtml(choices).c_str();
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
    html += "<form method='get' action='/display#text-preview'><label for='preview-source'>Source</label>";
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

String buildPropertyPagePreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const PropertyPreviewPage& page, bool submitted) {
    String html("<section class='card' id='text-preview'><h2>Six-line text preview</h2>");
    html += "<p>Each line has its own format and up to four sources, in placeholder order. Leave sources empty for plain text or an empty line.</p>";
    html += "<p class='help'>Optional Boolean text applies to %s: empty keeps the default text. Maximum 16 UTF-8 bytes each (umlauts use two bytes). Other source types ignore these labels.</p>";
    html += "<template id='property-sources'><select>";
    html += optionsHtml.c_str();
    html += "</select></template><noscript><p>Enable JavaScript to choose display sources.</p></noscript><form method='get' action='/display#text-preview'>";
    html += "<fieldset><legend>OLED SSD1309 · 128×64</legend><label>Display output<select name='displayEnabled'>";
    html += page.hardware.enabled ? "<option value='0'>Disabled</option><option value='1' selected>Enabled</option>"
        : "<option value='0' selected>Disabled</option><option value='1'>Enabled</option>";
    html += "</select></label><label>I2C bus<select name='displayBus'>";
    const auto& board = BoardCapabilities::current();
    for (size_t index = 0; index < board.i2cBusCount(); ++index) {
        const auto* bus = board.i2cBusAt(index);
        html += "<option value='";
        html += String(static_cast<unsigned int>(bus->bus)).c_str();
        html += bus->bus == page.hardware.bus ? "' selected>" : "'>";
        html += i2cBusName(bus->bus);
        html += " (SDA "; html += String(static_cast<unsigned int>(bus->sda.number)).c_str();
        html += ", SCL "; html += String(static_cast<unsigned int>(bus->scl.number)).c_str();
        html += ")</option>";
    }
    html += "</select></label><label>I2C address<select name='displayAddress'>";
    html += page.hardware.address == 0x3C
        ? "<option value='60' selected>0x3C</option><option value='61'>0x3D</option>"
        : "<option value='60'>0x3C</option><option value='61' selected>0x3D</option>";
    html += "</select></label><p class='help'>Save on device applies output settings immediately. Refresh: 1 second. Six lines, 10-pixel spacing; long lines are clipped at the right edge. Connection errors are logged; retry: 10 seconds.</p></fieldset>";
    for (size_t line = 0; line < PropertyPreviewLineCount; ++line) {
        const String number(static_cast<unsigned int>(line));
        html += "<fieldset><legend>Line ";
        html += String(static_cast<unsigned int>(line + 1)).c_str();
        html += "</legend><label for='f";
        html += number.c_str();
        html += "'>Format</label><input id='f";
        html += number.c_str();
        html += "' name='f";
        html += number.c_str();
        html += "' maxlength='128' value='";
        if (page.formats[line].length() <= MaxPropertyFormatLength) html += escapeHtml(page.formats[line]).c_str();
        html += "'>";
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            const String field = "s" + number + "_" + String(static_cast<unsigned int>(source));
            html += "<label for='";
            html += field.c_str();
            html += "'>Source ";
            html += String(static_cast<unsigned int>(source + 1)).c_str();
            html += "</label><select data-property-source id='";
            html += field.c_str();
            html += "' name='";
            html += field.c_str();
            html += "'><option value=''>No source</option>";
            if (!page.sources[line][source].isEmpty()
                && page.sources[line][source].length() <= MaxPropertySourceLength) {
                html += "<option selected value='";
                html += escapeHtml(page.sources[line][source]).c_str();
                html += "'>";
                html += escapeHtml(page.sources[line][source]).c_str();
                html += "</option>";
            }
            html += "</select>";
            const auto& labels = page.labels[line][source];
            PropertySourceInput selected;
            PropertyDescription description;
            const bool isBoolean = parsePropertySource(page.sources[line][source].c_str(), selected)
                && reader.describe(selected.reference(), description)
                && description.valueKind == PropertyValueKind::Boolean;
            html += "<details data-boolean-source='";
            html += field.c_str();
            html += "'";
            if (!isBoolean) html += " hidden";
            if (!labels.trueText.isEmpty() || !labels.falseText.isEmpty()) html += " open";
            html += ">";
            html += "<summary>Boolean text (optional)</summary>";
            const String suffix = number + "_" + String(static_cast<unsigned int>(source));
            const auto labelInput = [&](const char* prefix, const char* title, const String& value) {
                html += "<label>"; html += title;
                html += "<input name='"; html += prefix; html += suffix.c_str();
                html += "' maxlength='16' value='";
                if (value.length() <= MaxDisplayBooleanLabelLength) html += escapeHtml(value).c_str();
                html += "'></label>";
            };
            labelInput("true", "True / On text", labels.trueText);
            labelInput("false", "False / Off text", labels.falseText);
            html += "</details>";
            html += "<details data-enum-source='"; html += field.c_str(); html += "'";
            const bool isEnum = description.valueKind == PropertyValueKind::Enumeration;
            if (!isEnum) html += " hidden";
            html += "><summary>State text (optional)</summary><div data-enum-fields>";
            if (isEnum) for (size_t index = 0; index < description.enumOptionCount; ++index) {
                const auto& option = description.enumOptions[index];
                html += "<label>"; html += escapeHtml(String(option.displayText)).c_str();
                html += "<input maxlength='16' data-code='"; html += escapeHtml(String(option.stableCode)).c_str();
                html += "' name='e"; html += suffix.c_str(); html += "_"; html += escapeHtml(String(option.stableCode)).c_str();
                html += "' value='"; html += escapeHtml(String(displayEnumText(page, line, source, option.stableCode))).c_str();
                html += "'></label>";
            }
            html += "</div><p class='help'>Empty keeps the original state text. Up to 32 translations per page, 16 UTF-8 bytes each.</p></details>";
        }
        html += "</fieldset>";
    }
    html += "<div class='actions'><button type='submit' name='preview' value='1'>Preview / refresh all lines</button><button type='submit' formmethod='post' formaction='/display/save'>Save on device</button><button type='submit' form='display-load'>Load saved settings</button></div></form><form id='display-load' method='get' action='/display#text-preview'><input type='hidden' name='loadDisplay' value='1'></form>";
    html += "<p class='help'>Use %f for decimals, %u for unsigned integers (including actuator percent), %s for text, Boolean or enum text, and %% for a percent sign. Example: Level: %.0f l %.0f%% with two sources. Width up to 64; decimal places 0–6. Numbers use canonical units and a decimal point.</p>";
    html += "<p class='help'>This is a six-line text preview, not a pixel-accurate OLED simulation. Preview settings remain in this URL until you choose Save on device. Saved settings survive restart. Load saved settings discards the preview edits. Refresh reads all lines again.</p>";
    if (optionsHtml.isEmpty()) html += "<p>No sources available; plain text still works.</p>";
    if (submitted) {
        // Render a complete page only after each row has been evaluated. A bad
        // row gets an explicit marker; its previous or partial value is never reused.
        String rendered;
        for (size_t line = 0; line < PropertyPreviewLineCount; ++line) {
            const DisplayLineResult lineResult = formatDisplayLine(reader, page, line);
            const char* error = lineResult.error;
            const PropertyTextResult& result = lineResult.value;
            if (error != nullptr) {
                rendered += "[Line ";
                rendered += String(static_cast<unsigned int>(line + 1)).c_str();
                rendered += ": ";
                rendered += error;
                rendered += "]";
            } else {
                rendered += result.text;
            }
            rendered += '\n';
        }
        html += "<h3>Display text</h3><pre style='white-space:pre;overflow-x:auto'>";
        html += escapeHtml(rendered).c_str();
        html += "</pre>";
    }
    html += R"HTML(<script>
(function () {
    const options = document.getElementById('property-sources').content.querySelector('select').options;
    document.querySelectorAll('[data-property-source]').forEach(source => {
        const selected = source.value;
        const parts = selected.split('/');
        if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
        const canonical = parts.join('/');
        source.replaceChildren(new Option('No source', ''));
        Array.from(options).forEach(option => source.appendChild(option.cloneNode(true)));
        const available = Array.from(source.options).some(option => option.value === canonical);
        if (!available) source.add(new Option('Unavailable: ' + selected, selected));
        source.value = available ? canonical : selected;
    });
    const types = new Map(Array.from(options, option => [option.value, option.dataset.boolean === '1']));
    document.querySelectorAll('[data-boolean-source]').forEach(panel => {
        const source = document.getElementById(panel.dataset.booleanSource);
        const update = () => {
            const parts = source.value.split('/');
            if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
            panel.hidden = types.get(parts.join('/')) !== true;
        };
        source.addEventListener('input', update);
        source.addEventListener('change', update);
    });
    const states = new Map(Array.from(options, option => [option.value, JSON.parse(option.dataset.enum || '[]')]));
    document.querySelectorAll('[data-enum-source]').forEach(panel => {
        const source = document.getElementById(panel.dataset.enumSource);
        const fields = panel.querySelector('[data-enum-fields]');
        const values = new Map();
        const update = () => {
            fields.querySelectorAll('input').forEach(input => values.set(input.dataset.code, input.value));
            const parts = source.value.split('/');
            if (parts.length === 3 && /^[0-9]+$/.test(parts[1])) parts[1] = String(Number(parts[1]));
            const choices = states.get(parts.join('/')) || [];
            panel.hidden = choices.length === 0;
            if (!choices.length) return;
            fields.replaceChildren();
            choices.forEach(([code, text]) => {
                const label = document.createElement('label'); label.textContent = text;
                const input = document.createElement('input');
                input.name = 'e' + source.id.slice(1) + '_' + code;
                input.dataset.code = code; input.maxLength = 16; input.value = values.get(code) || '';
                label.appendChild(input); fields.appendChild(label);
            });
        };
        source.addEventListener('input', update);
        source.addEventListener('change', update);
    });
})();
</script>)HTML";
    html += "</section>";
    return html;
}

} // namespace EnvNode
