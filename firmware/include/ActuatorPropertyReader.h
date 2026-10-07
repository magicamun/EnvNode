#pragma once

#include "ActuatorId.h"
#include "IOnOffActuator.h"
#include "ILevelActuator.h"
#include "IPropertyReader.h"

namespace EnvNode {

// Borrowed, read-only binding. Recreate after an Actuator runtime rebuild.
class ActuatorPropertyReader : public IPropertyReader {
public:
    ActuatorPropertyReader(ActuatorId id, const IOnOffActuator& actuator, const ILevelActuator* level = nullptr);
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override;

private:
    bool matches(const PropertyReference& reference) const;
    ActuatorId id_;
    const IOnOffActuator& actuator_;
    const ILevelActuator* level_;
};

} // namespace EnvNode
