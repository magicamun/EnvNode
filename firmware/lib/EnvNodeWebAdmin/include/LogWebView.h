#pragma once
#include "WebLocalization.h"

#include <Arduino.h>

#include "IRecentLogReader.h"

namespace EnvNode {

String buildRecentLogHtml(const IRecentLogReader& logReader, Locale locale = Locale::EnglishUnitedKingdom, size_t offset=0, size_t limit=static_cast<size_t>(-1));
String buildRecentLogJson(const IRecentLogReader& logReader, size_t offset=0, size_t limit=static_cast<size_t>(-1));
const char* recentLogScript();

} // namespace EnvNode
