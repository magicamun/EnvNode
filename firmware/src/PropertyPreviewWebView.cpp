#include "PropertyPreviewWebView.h"
#include "DisplayPageFormatter.h"
#include "TextDisplayLayout.h"
#include <cstdio>

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
    html += "' data-format='";
    html += description.valueKind == PropertyValueKind::FloatingPoint ? "f"
        : description.valueKind == PropertyValueKind::UnsignedInteger ? "u" : "s";
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
    html += "'><button disabled data-editor-submit type='submit' name='preview' value='1'>Preview / refresh</button></form>";
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

String buildDisplayTabs(size_t selected) {
    String html("<nav class='actions' aria-label='Display configuration'>");
    const char* titles[] = {"Displays", "Page 1", "Page 2"};
    const char* urls[] = {"/display", "/display?page=1", "/display?page=2"};
    for (size_t i = 0; i < 3; ++i) {
        html += "<a class='button' href='"; html += urls[i];
        html += i == selected ? "' aria-current='page' style='font-weight:700;text-decoration:underline'>" : "'>";
        html += titles[i]; html += "</a>";
    }
    html += "</nav>";
    return html;
}

String buildDisplayOutputsHtml(const DisplayConfiguration& page) {
    String html("<section class='card'>");
    html += buildDisplayTabs(0).c_str();
    html += "<h2>Displays</h2><p class='help'>Configure hardware and assign pages here. Page contents are edited separately.</p><form method='post' action='/display/outputs/save'>";
    for (size_t output = 0; output < 2; ++output) {
        const auto& hardware = page.output(output);
        const String suffix = output == 0 ? "" : "2";
        html += "<fieldset><legend>Display "; html += String(static_cast<unsigned>(output + 1)).c_str(); html += "</legend><label>Display output<select name='displayEnabled"; html += suffix.c_str(); html += "'>";
        html += hardware.enabled ? "<option value='0'>Disabled</option><option value='1' selected>Enabled</option>"
            : "<option value='0' selected>Disabled</option><option value='1'>Enabled</option>";
        html += "</select></label><label>Display type<select name='displayType"; html += suffix.c_str(); html += "'>";
        for (unsigned int value = 0; value < 3; ++value) {
            const auto type = static_cast<TextDisplayType>(value);
            html += "<option value='"; html += String(value).c_str();
            html += type == hardware.type ? "' selected>" : "'>";
            html += textDisplayTypeName(type); html += "</option>";
        }
        html += "</select></label><label>Font<select name='displayFont"; html += suffix.c_str(); html += "'>";
        for (unsigned value = 0; value < 2; ++value) {
            auto candidate = hardware; candidate.font = static_cast<TextDisplayFont>(value);
            const auto layout = textDisplayLayout(candidate);
            html += "<option value='"; html += String(value).c_str();
            html += candidate.font == hardware.font ? "' selected>" : "'>";
            html += textDisplayFontName(candidate.font); html += " · ";
            html += String(static_cast<unsigned>(layout.columns())).c_str(); html += " columns x ";
            html += String(static_cast<unsigned>(layout.rows())).c_str(); html += " lines</option>";
        }
        html += "</select></label><label>I2C bus<select name='displayBus"; html += suffix.c_str(); html += "'>";
        const auto& board = BoardCapabilities::current();
        for (size_t index = 0; index < board.i2cBusCount(); ++index) {
            const auto* bus = board.i2cBusAt(index);
            html += "<option value='";
            html += String(static_cast<unsigned int>(bus->bus)).c_str();
            html += bus->bus == hardware.bus ? "' selected>" : "'>";
            html += i2cBusName(bus->bus);
            html += " (SDA "; html += String(static_cast<unsigned int>(bus->sda.number)).c_str();
            html += ", SCL "; html += String(static_cast<unsigned int>(bus->scl.number)).c_str();
            html += ")</option>";
        }
        html += "</select></label><label>I2C address<select name='displayAddress"; html += suffix.c_str(); html += "'>";
        html += hardware.address == 0x3C
            ? "<option value='60' selected>0x3C</option><option value='61'>0x3D</option>"
            : "<option value='60'>0x3C</option><option value='61' selected>0x3D</option>";
        html += "</select></label><p class='help'>Save on device applies output settings immediately. Refresh: 1 second. Font determines columns and lines. Output is clipped to complete cells without wrapping; page content is retained. Connection errors are logged; retry: 10 seconds.</p>";
        html += "<label>Page<select name='displayPage"; html += suffix.c_str(); html += "'>";
        for (unsigned i = 0; i < 2; ++i) {
            html += "<option value='"; html += String(i).c_str();
            html += page.pageAssignment[output] == i ? "' selected>" : "'>";
            html += "Page "; html += String(i + 1).c_str(); html += "</option>";
        }
        html += "</select></label>";
        html += "</fieldset>";
    }
    html += "<div class='actions'><button type='submit'>Save on device</button><button type='submit' form='display-outputs-load'>Load saved settings</button></div></form><form id='display-outputs-load' method='get' action='/display'></form></section>";
    return html;
}

String buildPropertyPagePreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const PropertyPreviewPage& page, bool submitted, size_t pageIndex, int previewOutput) {
    const auto& contentPage = page.page(pageIndex);
    String html("<section class='card' id='text-preview'><h2>Six-line text preview</h2>");
    html += buildDisplayTabs(pageIndex + 1).c_str();
    html += "<h3>Page "; html += String(static_cast<unsigned>(pageIndex + 1)).c_str();
    html += "</h3><p class='help'>Save edits before switching tabs. Only this page's content is saved here.</p>";
    html += "<p>Enter up to six format lines on the left. Choose a source for each placeholder below; plain text and %% need no source.</p>";
    html += "<p class='help'>Optional Boolean text applies to %s: empty keeps the default text. Maximum 16 UTF-8 bytes each (umlauts use two bytes). Other source types ignore these labels.</p>";
    html += "<template id='property-sources'><select>";
    html += optionsHtml.c_str();
    html += "</select></template><noscript><p>Enable JavaScript to edit and save display pages.</p></noscript><form method='get' action='/display#text-preview'>";
    html += "<input type='hidden' name='page' value='"; html += String(static_cast<unsigned>(pageIndex + 1)).c_str(); html += "'>";
    html += "<style>#text-preview [hidden]{display:none!important}.page-editor-grid{display:grid;grid-template-columns:minmax(0,1fr) minmax(0,1fr);gap:16px}.page-editor-grid textarea{box-sizing:border-box;width:100%;max-width:100%;font-family:monospace;white-space:pre;min-height:11em;resize:vertical}[data-page-row]>legend{white-space:nowrap;width:auto;padding:0 6px}.page-source-fields{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:12px}#page-editor-note{min-height:1.5em}@media(max-width:700px){.page-editor-grid{grid-template-columns:1fr}}</style>";
    html += "<div class='page-editor-grid'><label>Format / one line per display line<textarea id='page-formats' disabled rows='6' wrap='off' spellcheck='false'>";
    html += "</textarea></label><div><label>Preview<textarea id='page-result' rows='6' wrap='off' readonly aria-label='Display preview'></textarea></label><label>Preview for<select id='preview-output' name='previewOutput'>";
    if (previewOutput == -1) for (size_t output = 0; output < 2; ++output) {
        if (page.output(output).enabled && page.pageAssignment[output] == pageIndex) { previewOutput = output; break; }
    }
    html += "<option value='full'"; html += previewOutput < 0 ? " selected>" : ">";
    html += "Full page (unclipped)</option>";
    for (size_t output = 0; output < 2; ++output) {
        if (page.pageAssignment[output] != pageIndex) continue;
        const auto layout = textDisplayLayout(page.output(output));
        html += "<option value='"; html += String(static_cast<unsigned>(output)).c_str();
        html += previewOutput == static_cast<int>(output) ? "' selected>" : "'>";
        html += "Display "; html += String(static_cast<unsigned>(output + 1)).c_str(); html += " · ";
        html += String(static_cast<unsigned>(layout.columns())).c_str(); html += " columns x ";
        html += String(static_cast<unsigned>(layout.rows())).c_str(); html += " lines";
        if (!page.output(output).enabled) html += " (disabled)";
        html += "</option>";
    }
    html += "</select></label></div></div><p id='page-editor-note' class='help' role='status'></p><div id='page-sources'>";
    for (size_t line = 0; line < PropertyPreviewLineCount; ++line) {
        const String number(static_cast<unsigned int>(line));
        html += "<fieldset data-page-row><legend>Line ";
        html += String(static_cast<unsigned int>(line + 1)).c_str();
        html += "</legend><div class='page-source-fields'><label hidden for='f";
        html += number.c_str();
        html += "'>Format</label><input hidden data-format id='f";
        html += number.c_str();
        html += "' name='f";
        html += number.c_str();
        html += "' maxlength='128' value='";
        if (contentPage.formats[line].length() <= MaxPropertyFormatLength) html += escapeHtml(contentPage.formats[line]).c_str();
        html += "'>";
        for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
            const String field = "s" + number + "_" + String(static_cast<unsigned int>(source));
            html += "<div data-source-slot><label for='";
            html += field.c_str();
            html += "'>Source ";
            html += String(static_cast<unsigned int>(source + 1)).c_str();
            html += "</label><select data-property-source id='";
            html += field.c_str();
            html += "' name='";
            html += field.c_str();
            html += "'><option value=''>No source</option>";
            if (!contentPage.sources[line][source].isEmpty()
                && contentPage.sources[line][source].length() <= MaxPropertySourceLength) {
                html += "<option selected value='";
                html += escapeHtml(contentPage.sources[line][source]).c_str();
                html += "'>";
                html += escapeHtml(contentPage.sources[line][source]).c_str();
                html += "</option>";
            }
            html += "</select>";
            const auto& labels = contentPage.labels[line][source];
            PropertySourceInput selected;
            PropertyDescription description;
            const bool isBoolean = parsePropertySource(contentPage.sources[line][source].c_str(), selected)
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
                html += "' value='"; html += escapeHtml(String(displayEnumText(contentPage, line, source, option.stableCode))).c_str();
                html += "'></label>";
            }
            html += "</div><p class='help'>Empty keeps the original state text. Up to 32 translations per page, 16 UTF-8 bytes each.</p></details></div>";
        }
        html += "</div></fieldset>";
    }
    html += "</div><div class='actions'><button disabled data-editor-submit type='submit' name='preview' value='1'>Preview / refresh all lines</button><button disabled data-editor-submit type='submit' formmethod='post' formaction='/display/save'>Save on device</button><button type='submit' form='display-load'>Load saved settings</button></div></form><form id='display-load' method='get' action='/display#text-preview'><input type='hidden' name='loadDisplay' value='1'><input type='hidden' name='page' value='"; html += String(static_cast<unsigned>(pageIndex + 1)).c_str(); html += "'></form>";
    html += "<p class='help'>Use %f for decimals, %u for unsigned integers (including actuator percent), %s for text, Boolean or enum text, and %% for a percent sign. Example: Level: %.0f l %.0f%% with two sources. Width up to 64; decimal places 0–6. Numbers use canonical units and a decimal point.</p>";
    html += "<p class='help'>Preview uses the selected display font limits, not a pixel-accurate OLED simulation. Full page shows all stored text. Unsupported display characters appear as ?. Preview settings remain in this URL until you choose Save on device. Saved settings survive restart. Load saved settings discards the preview edits. Refresh reads all lines again.</p>";
    if (optionsHtml.isEmpty()) html += "<p>No sources available; plain text still works.</p>";
    if (submitted) {
        // Render a complete page only after each row has been evaluated. A bad
        // row gets an explicit marker; its previous or partial value is never reused.
        String rendered;
        String clipped[2];
        for (size_t line = 0; line < PropertyPreviewLineCount; ++line) {
            const DisplayLineResult lineResult = formatDisplayLine(reader, contentPage, line);
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
            for (size_t output = 0; output < 2; ++output) {
                if (page.pageAssignment[output] != pageIndex) continue;
                const auto layout = textDisplayLayout(page.output(output));
                if (line >= layout.rows()) continue;
                char text[MaxPropertyTextLength + 1];
                if (error) {
                    snprintf(text, sizeof(text), "[Line %u error]", static_cast<unsigned>(line + 1));
                    clipDisplayText(text, layout.columns(), text, sizeof(text));
                } else clipDisplayText(result.text, layout.columns(), text, sizeof(text));
                clipped[output] += text; clipped[output] += '\n';
            }
        }
        html += "<pre hidden id='page-rendered'>\n";
        html += escapeHtml(rendered).c_str();
        html += "</pre>";
        for (size_t output = 0; output < 2; ++output) {
            if (page.pageAssignment[output] != pageIndex) continue;
            html += "<pre hidden id='page-rendered-"; html += String(static_cast<unsigned>(output)).c_str(); html += "'>\n";
            html += escapeHtml(clipped[output]).c_str(); html += "</pre>";
        }
    }
    html += "<script src='/display-editor.js' defer></script>";
    html += "</section>";
    return html;
}

} // namespace EnvNode
