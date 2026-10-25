// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include <Arduino.h>
#include "EnvNode/Locale.h"
namespace EnvNode {
enum class TextId : uint16_t {
#define ENVNODE_WEB_TEXT(id, english, german) id,
#include "WebTextCatalog.inc"
#undef ENVNODE_WEB_TEXT
    Count
};
// Missing translations use English. Invalid IDs return an empty string.
const char* webText(Locale locale, TextId id);
const char* webLanguageTag(Locale locale);
String buildWebUiScript(Locale locale);
} // namespace EnvNode
