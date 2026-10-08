#pragma once
#include "IConfigurationService.h"
#include "IEnumValueReader.h"

namespace EnvNode {
enum class ValueCommandResult { Changed, Unchanged, UnknownValue, UnknownOption, StorageFailed };
class ValueRuntime : public IEnumValueReader {
public:
    explicit ValueRuntime(IConfigurationService& configuration) : configuration_(configuration) {}
    void begin();
    uint32_t compositionRevision() const { return compositionRevision_; }
    const EnumValueConfiguration* valueDefinition(ValueId id) const override;
    bool valueCode(ValueId id, String& code) const override;
    const std::vector<EnumValue>& values() const { return values_; }
    const EnumValue* find(ValueId id) const;
    ValueCommandResult set(ValueId id, const String& code);
    bool saveDefinition(const EnumValueConfiguration& definition, bool create);
    bool remove(ValueId id);
private:
    IConfigurationService& configuration_;
    std::vector<EnumValue> values_;
    uint32_t compositionRevision_ = 0;
};
} // namespace EnvNode
