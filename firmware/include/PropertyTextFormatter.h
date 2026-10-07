#pragma once

#include <cstddef>
#include "IPropertyReader.h"

namespace EnvNode {

constexpr size_t MaxPropertyFormatLength = 128;
constexpr size_t MaxPropertyTextLength = 128;
constexpr size_t MaxPropertySourcesPerLine = 4;

enum class PropertyFormatStatus {
    Formatted,
    InvalidFormat,
    FormatTooLong,
    TypeMismatch,
    UnknownReference,
    NoValue,
    InvalidValue,
    OutputTooLong,
    SourceCountMismatch,
};

struct PropertyTextResult {
    PropertyFormatStatus status = PropertyFormatStatus::InvalidFormat;
    char text[MaxPropertyTextLength + 1] = {};
};

// Compatibility overload: exactly one typed conversion (%f, %u or %s), plus literal text and %%.
// All printf calls use trusted literal formats, never the supplied format.
PropertyTextResult formatPropertyText(const IPropertyReader& reader,
    const PropertyReference& reference, const char* format);
// Zero to four ordered references; zero allows literal text and empty lines.
PropertyTextResult formatPropertyText(const IPropertyReader& reader,
    const PropertyReference* references, size_t referenceCount, const char* format);
bool validatePropertyFormat(const char* format, size_t sourceCount);
const char* propertyFormatError(PropertyFormatStatus status);

} // namespace EnvNode
