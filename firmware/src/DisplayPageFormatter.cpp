#include "DisplayPageFormatter.h"
#include "PropertySourceInput.h"
namespace EnvNode {
DisplayLineResult formatDisplayLine(const IPropertyReader& reader,
    const DisplayConfiguration& page, size_t line) {
    DisplayLineResult result;
    if (line >= DisplayLineCount) { result.error = "Invalid line."; return result; }
    PropertySourceInput parsed[MaxPropertySourcesPerLine];
    PropertyReference references[MaxPropertySourcesPerLine] = {
        {PropertyComponentKind::Unknown, 0, nullptr}, {PropertyComponentKind::Unknown, 0, nullptr},
        {PropertyComponentKind::Unknown, 0, nullptr}, {PropertyComponentKind::Unknown, 0, nullptr}};
    size_t count = 0;
    bool gap = false;
    for (size_t source = 0; source < MaxPropertySourcesPerLine; ++source) {
        if (page.sources[line][source].isEmpty()) { gap = true; continue; }
        if (gap) { result.error = "Fill sources in order without gaps."; return result; }
        if (!parsePropertySource(page.sources[line][source].c_str(), parsed[source])) {
            result.error = "Invalid source: use category/ID/property.";
            return result;
        }
        references[count++] = parsed[source].reference();
    }
    result.value = formatPropertyText(reader, references, count, page.formats[line].c_str());
    if (result.value.status != PropertyFormatStatus::Formatted) result.error = propertyFormatError(result.value.status);
    return result;
}
}
