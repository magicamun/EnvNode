#pragma once

#include "Measurement.h"

namespace EnvNode {

class IMeasurementSink {
public:
    virtual ~IMeasurementSink() = default;

    // Synchronously consumes or copies the content. Implementations must not
    // retain the reference after this call returns.
    virtual void emit(const Measurement& measurementContent) = 0;
};

} // namespace EnvNode
