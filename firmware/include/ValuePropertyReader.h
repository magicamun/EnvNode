#pragma once
#include "IPropertyReader.h"
#include "ValueRuntime.h"
namespace EnvNode {
class ValuePropertyReader : public IPropertyReader {
public:
    explicit ValuePropertyReader(const ValueRuntime& runtime) : runtime_(runtime) {}
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override;
private:
    const EnumValue* resolve(const PropertyReference& reference) const;
    const ValueRuntime& runtime_;
};
} // namespace EnvNode
