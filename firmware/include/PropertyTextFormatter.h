#pragma once

#include <cstddef>
#include "IPropertyReader.h"

namespace EnvNode {

constexpr size_t MaxPropertyFormatLength = 128;
constexpr size_t MaxPropertyTextLength = 128;

enum class PropertyFormatStatus {
    Formatted,
    InvalidFormat,
    FormatTooLong,
    TypeMismatch,
    UnknownReference,
    NoValue,
    InvalidValue,
    OutputTooLong,
};

struct PropertyTextResult {
    PropertyFormatStatus status = PropertyFormatStatus::InvalidFormat;
    char text[MaxPropertyTextLength + 1] = {};
};

// Exactly one typed conversion (%f, %u or %s), plus literal text and %%.
// All printf calls use trusted literal formats, never the supplied format.
PropertyTextResult formatPropertyText(const IPropertyReader& reader,
    const PropertyReference& reference, const char* format);
const char* propertyFormatError(PropertyFormatStatus status);

} // namespace EnvNode
