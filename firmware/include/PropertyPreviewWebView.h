#pragma once

#include <Arduino.h>
#include "IPropertyReader.h"

namespace EnvNode {

constexpr size_t MaxPropertySourceLength = 96;
struct PropertySourceInput {
    PropertyComponentKind kind = PropertyComponentKind::Unknown;
    uint16_t id = 0;
    char key[65] = {};
    PropertyReference reference() const { return PropertyReference(kind, id, key); }
};

bool parsePropertySource(const char* text, PropertySourceInput& result);
String propertySourceText(const PropertyReference& reference);
String buildPropertySourceOption(const PropertyReference& reference,
    const PropertyDescription& description, const char* componentName, const String& selectedSource);
// optionsHtml must come only from buildPropertySourceOption(), never request data.
String buildPropertyPreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const String& source, const String& format, bool submitted, bool sourceListed);

} // namespace EnvNode
