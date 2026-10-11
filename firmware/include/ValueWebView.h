#pragma once
#include "WebLocalization.h"
#include "ValueRuntime.h"
namespace EnvNode {
bool parseValueId(const String& text, ValueId& id);
String buildValuesHtml(const ValueRuntime& runtime, Locale locale = Locale::EnglishUnitedKingdom);
String buildValueEditorHtml(const EnumValueConfiguration& definition, bool create, const char* error = nullptr, Locale locale = Locale::EnglishUnitedKingdom);
} // namespace EnvNode
