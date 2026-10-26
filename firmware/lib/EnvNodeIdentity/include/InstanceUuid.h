#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

class InstanceUuid {
public:
    static constexpr size_t Size = 16;

    // Converts 128 random bits into an RFC 9562 UUID version 4.
    static void makeVersion4(uint8_t (&value)[Size]);
};

} // namespace EnvNode
