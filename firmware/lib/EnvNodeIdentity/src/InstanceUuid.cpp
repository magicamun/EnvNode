#include "InstanceUuid.h"

namespace EnvNode {

void InstanceUuid::makeVersion4(uint8_t (&value)[Size]) {
    value[6] = static_cast<uint8_t>((value[6] & 0x0F) | 0x40);
    value[8] = static_cast<uint8_t>((value[8] & 0x3F) | 0x80);
}

} // namespace EnvNode
