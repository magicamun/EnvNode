#pragma once

#include "ControllerId.h"
#include "IPropertyReader.h"
#include "IThresholdReasonProvider.h"

namespace EnvNode {

// Borrowed binding; recreate when the runtime composition changes.
class ControllerPropertyReader : public IPropertyReader {
public:
    ControllerPropertyReader(ControllerId id, const IThresholdReasonProvider& provider);
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override;
private:
    bool matches(const PropertyReference& reference) const;
    ControllerId id_;
    const IThresholdReasonProvider& provider_;
};

} // namespace EnvNode
