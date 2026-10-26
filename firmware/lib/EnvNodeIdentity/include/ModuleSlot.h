#pragma once

#include <cstdint>

namespace EnvNode {

enum class ModuleSlot : uint8_t {
    A = 0,
    B = 1,
};

const char* moduleSlotName(ModuleSlot slot);

} // namespace EnvNode
