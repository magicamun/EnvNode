#pragma once
#include "WebLocalization.h"

#include <Arduino.h>
#include "IPropertyReader.h"
#include "PropertyTextFormatter.h"
#include "PropertySourceInput.h"
#include "DisplayConfiguration.h"

namespace EnvNode {

String buildDisplayTabs(size_t selected, Locale locale = Locale::EnglishUnitedKingdom);
String buildDisplayOutputsHtml(const DisplayConfiguration& configuration, Locale locale = Locale::EnglishUnitedKingdom);
String propertySourceText(const PropertyReference& reference);
String buildPropertySourceOption(const PropertyReference& reference,
    const PropertyDescription& description, const char* componentName, const String& selectedSource, Locale locale = Locale::EnglishUnitedKingdom);
// optionsHtml must come only from buildPropertySourceOption(), never request data.
String buildPropertyPreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const String& source, const String& format, bool submitted, bool sourceListed, Locale locale = Locale::EnglishUnitedKingdom);

constexpr size_t PropertyPreviewLineCount = DisplayLineCount;
using PropertyPreviewPage = DisplayConfiguration;
// optionsHtml contains escaped options produced by buildPropertySourceOption.
String buildPropertyPagePreviewHtml(const IPropertyReader& reader, const String& optionsHtml,
    const PropertyPreviewPage& page, bool submitted, size_t pageIndex = 0, int previewOutput = -1, Locale locale = Locale::EnglishUnitedKingdom);

} // namespace EnvNode
