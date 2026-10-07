#pragma once
#include "IPropertyReader.h"
#include "ITimeService.h"
#include "LocaleFormatter.h"
namespace EnvNode {
class TimePropertyReader : public IPropertyReader {
public:
    TimePropertyReader(const ITimeService& time, const LocaleFormatter& locale) : time_(time), locale_(locale) {}
    bool describe(const PropertyReference& reference, PropertyDescription& result) const override;
    PropertyReadResult read(const PropertyReference& reference, PropertySnapshot& result) const override;
private:
    const ITimeService& time_;
    const LocaleFormatter& locale_;
};
}
