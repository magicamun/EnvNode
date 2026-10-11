#pragma once
#include "WebLocalization.h"

#include <Arduino.h>
#include "IPropertyReader.h"

namespace EnvNode {

String buildPropertyDiagnosticHtml(const IPropertyReader& reader,
    const PropertyReference& reference, uint32_t nowMs, Locale locale = Locale::EnglishUnitedKingdom);

} // namespace EnvNode
