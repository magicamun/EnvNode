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

namespace EnvNode {
struct ControllerRuntimeInfo;
class LocaleFormatter;
// Borrowed copied runtime snapshot; no driver dependency or hardware side effects.
class RainDetectorPropertyReader : public IPropertyReader {
public:
    explicit RainDetectorPropertyReader(const ControllerRuntimeInfo& info, const LocaleFormatter* locale = nullptr) : info_(info), locale_(locale) {}
    static const char* const* keys(size_t& count);
    bool describe(const PropertyReference&, PropertyDescription&) const override;
    PropertyReadResult read(const PropertyReference&, PropertySnapshot&) const override;
private:
    const ControllerRuntimeInfo& info_;
    const LocaleFormatter* locale_;
};
}
