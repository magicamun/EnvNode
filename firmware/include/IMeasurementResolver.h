#pragma once

#include "MeasurementSnapshot.h"
#include "MeasurementSourceReference.h"

namespace EnvNode {

class IMeasurementResolver {
public:
    virtual ~IMeasurementResolver() = default;

    virtual bool latest(
        const MeasurementSourceReference& source,
        MeasurementSnapshot& result) const = 0;
};

} // namespace EnvNode
