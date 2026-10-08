#include "DisplayPageFormatter.h"
#include "PropertySourceInput.h"
namespace EnvNode {
DisplayLineResult formatDisplayLine(const IPropertyReader& reader,
    const DisplayPage& page, size_t line) {
    DisplayLineResult result;
    if (line >= DisplayLineCount) { result.error = "Invalid line."; return result; }
    PropertySourceInput parsed[MaxPropertySourcesPerLine];
    PropertyReference references[MaxPropertySourcesPerLine] = {
        {PropertyComponentKind::Unknown, 0, nullptr}, {PropertyComponentKind::Unknown, 0, nullptr},
        {PropertyComponentKind::Unknown, 0, nullptr}, {PropertyComponentKind::Unknown, 0, nullptr}};
    PropertyBooleanLabels labels[MaxPropertySourcesPerLine];
    PropertyEnumText entries[MaxDisplayEnumTranslations];
    PropertyEnumLabels enums[MaxPropertySourcesPerLine];
    size_t used = 0;
    size_t count = 0;
    bool gap = false;
    for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
        if (page.sources[line][source].isEmpty()) {
            if (!page.labels[line][source].trueText.isEmpty() || !page.labels[line][source].falseText.isEmpty()) {
                result.error = "Boolean text requires a source.";
                return result;
            }
            gap = true;
            continue;
        }
        if (gap) { result.error = "Fill sources in order without gaps."; return result; }
        if (!parsePropertySource(page.sources[line][source].c_str(), parsed[source])) {
            result.error = "Invalid source: use category/ID/property.";
            return result;
        }
        const auto& configured = page.labels[line][source];
        if (!validateDisplayBooleanLabel(configured.trueText) || !validateDisplayBooleanLabel(configured.falseText)) {
            result.error = "Boolean text: maximum 16 UTF-8 bytes, no control characters.";
            return result;
        }
        labels[count].trueText = configured.trueText.c_str();
        labels[count].falseText = configured.falseText.c_str();
        enums[count].entries = entries + used;
        for (const auto& entry : page.enumTranslations) {
            if (entry.line != line || entry.source != source) continue;
            if (used == MaxDisplayEnumTranslations) { result.error = "Too many state translations."; return result; }
            entries[used].code = entry.code.c_str(); entries[used].text = entry.text.c_str();
            ++used; ++enums[count].count;
        }
        references[count++] = parsed[source].reference();
    }
    result.value = formatPropertyText(reader, references, count, page.formats[line].c_str(), labels, enums);
    if (result.value.status != PropertyFormatStatus::Formatted) result.error = propertyFormatError(result.value.status);
    return result;
}
}
