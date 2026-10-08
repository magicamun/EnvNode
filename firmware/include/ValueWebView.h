#pragma once
#include "ValueRuntime.h"
namespace EnvNode {
bool parseValueId(const String& text, ValueId& id);
String buildValuesHtml(const ValueRuntime& runtime);
String buildValueEditorHtml(const EnumValueConfiguration& definition, bool create, const char* error = nullptr);
} // namespace EnvNode
