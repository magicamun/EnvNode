#pragma once

#include "MeasurementType.h"

namespace EnvNode {

class UnitConverter {
public:
    static bool convert(
        MeasurementType type,
        float canonicalValue,
        PresentationUnit presentationUnit,
        float& presentationValue);

    static const char* symbol(PresentationUnit unit);
    static const char* displayName(PresentationUnit unit);
    static const char* stableKey(PresentationUnit unit);
    static bool parseStableKey(const char* key, PresentationUnit& unit);
};

} // namespace EnvNode
