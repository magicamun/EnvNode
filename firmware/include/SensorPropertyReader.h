#pragma once

#include "IMeasurementResolver.h"
#include "IPropertyReader.h"
#include "ISensor.h"

namespace EnvNode {

// A read-only view for one existing Sensor. Dependencies must outlive the reader;
// recreate the reader when the Sensor runtime is rebuilt.
class SensorPropertyReader : public IPropertyReader {
public:
    SensorPropertyReader(const ISensor& sensor, const IMeasurementResolver& measurements);

    bool describe(
        const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(
        const PropertyReference& reference, PropertySnapshot& result) const override;

private:
    MeasurementType resolve(const PropertyReference& reference) const;

    const ISensor& sensor_;
    const IMeasurementResolver& measurements_;
};

} // namespace EnvNode
