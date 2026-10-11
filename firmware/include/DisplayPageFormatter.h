#pragma once
#include "DisplayConfiguration.h"
namespace EnvNode {
enum class DisplayLineIssue { None, InvalidLine, BooleanWithoutSource, SourceGap, InvalidSource, InvalidBooleanText, TooManyTranslations, FormatError };
struct DisplayLineResult {
    DisplayLineIssue issue = DisplayLineIssue::None;
    PropertyTextResult value;
    const char* error = nullptr;
};
DisplayLineResult formatDisplayLine(const IPropertyReader& reader,
    const DisplayPage& page, size_t line);
}
