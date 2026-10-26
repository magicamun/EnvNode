// EnvNode: see repository LICENSE.
#include "ModuleSlot.h"
namespace EnvNode {
const char* moduleSlotName(ModuleSlot slot) {
    switch (slot) {
        case ModuleSlot::A: return "A";
        case ModuleSlot::B: return "B";
        default: return "unknown";
    }
}
}
