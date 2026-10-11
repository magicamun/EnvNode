#pragma once

#include <Arduino.h>
#include "EnvNode/Locale.h"

namespace EnvNode {

String buildWebNavigationHtml(const char* activeRoute,
    Locale locale = Locale::EnglishUnitedKingdom);

} // namespace EnvNode
