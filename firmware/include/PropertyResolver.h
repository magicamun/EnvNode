#pragma once

#include "IPropertyReader.h"

namespace EnvNode {

class SensorManager;
class IMeasurementResolver;
class ActuatorRuntime;
class ControllerRuntime;

// Runtimes must outlive this resolver. Component bindings are looked up afresh
// on each call, so the resolver can survive a runtime rebuild.
class PropertyResolver : public IPropertyReader {
public:
    PropertyResolver(const SensorManager& sensors, const IMeasurementResolver& measurements,
        const ActuatorRuntime& actuators, const ControllerRuntime& controllers,
        const IPropertyReader* system = nullptr, const IPropertyReader* values = nullptr);

    bool describe(const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override;

private:
    const SensorManager& sensors_;
    const IMeasurementResolver& measurements_;
    const ActuatorRuntime& actuators_;
    const ControllerRuntime& controllers_;
    const IPropertyReader* system_;
    const IPropertyReader* values_;
};

} // namespace EnvNode
