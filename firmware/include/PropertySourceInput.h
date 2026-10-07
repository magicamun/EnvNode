#pragma once
#include "IPropertyReader.h"

namespace EnvNode {
constexpr size_t MaxPropertySourceLength = 96;
struct PropertySourceInput {
    PropertyComponentKind kind = PropertyComponentKind::Unknown;
    uint16_t id = 0;
    char key[65] = {};
    PropertyReference reference() const { return PropertyReference(kind, id, key); }
};

bool parsePropertySource(const char* text, PropertySourceInput& result);

}
