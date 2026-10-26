#pragma once
#include "WebLocalization.h"

#include <Arduino.h>

#include "IRecentLogReader.h"

namespace EnvNode {

String buildRecentLogHtml(const IRecentLogReader& logReader, Locale locale = Locale::EnglishUnitedKingdom);
String buildRecentLogJson(const IRecentLogReader& logReader);

} // namespace EnvNode
