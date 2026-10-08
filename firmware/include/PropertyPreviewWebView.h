#pragma once

#include <Arduino.h>
#include "IPropertyReader.h"
#include "PropertyTextFormatter.h"
#include "PropertySourceInput.h"
#include "DisplayConfiguration.h"

namespace EnvNode {

String buildDisplayTabs(size_t selected);
String buildDisplayOutputsHtml(const DisplayConfiguration& configuration);
String propertySourceText(const PropertyReference& reference);
String buildPropertySourceOption(const PropertyReference& reference,
    const PropertyDescription& description, const char* componentName, const String& selectedSource);
// optionsHtml must come only from buildPropertySourceOption(), never request data.
String buildPropertyPreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const String& source, const String& format, bool submitted, bool sourceListed);

constexpr size_t PropertyPreviewLineCount = DisplayLineCount;
using PropertyPreviewPage = DisplayConfiguration;
// optionsHtml contains escaped options produced by buildPropertySourceOption.
String buildPropertyPagePreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const PropertyPreviewPage& page, bool submitted, size_t pageIndex = 0);

} // namespace EnvNode
