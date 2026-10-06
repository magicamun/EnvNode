#pragma once

#include <Arduino.h>
#include "IPropertyReader.h"

namespace EnvNode {

String buildPropertyDiagnosticHtml(const IPropertyReader& reader,
    const PropertyReference& reference, uint32_t nowMs);

} // namespace EnvNode
