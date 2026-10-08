#pragma once
#include "EnumValue.h"
namespace EnvNode {
class IEnumValueReader {
public:
    virtual ~IEnumValueReader() = default;
    virtual const EnumValueConfiguration* valueDefinition(ValueId id) const = 0;
    virtual bool valueCode(ValueId id, String& code) const = 0;
};
}
