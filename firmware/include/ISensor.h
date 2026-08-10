#pragma once

#include "IMeasurementSink.h"
#include "MeasurementType.h"
#include "SensorId.h"
#include "SensorOperationResult.h"
#include "SensorProvenance.h"
#include "SensorState.h"

namespace EnvNode {

class ISensor {
public:
    virtual ~ISensor() = default;

    virtual SensorId id() const = 0;
    virtual const char* type() const = 0;
    virtual SensorProvenance provenance() const = 0;
    virtual SensorState state() const = 0;
    virtual bool supports(MeasurementType type) const = 0;

    virtual void begin() = 0;
    virtual SensorOperationResult service(IMeasurementSink& output) = 0;
    virtual SensorOperationResult sample(IMeasurementSink& output) = 0;
};

} // namespace EnvNode
