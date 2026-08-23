#pragma once

#include <cstddef>
#include <cstdint>

#include "BoardIdentity.h"

namespace EnvNode {

class BoardIdentityCodec {
public:
    static constexpr size_t EncodedSize = 32;
    static constexpr uint8_t FormatVersion = 1;

    static BoardIdentityStatus encode(
        const BoardIdentity& identity,
        uint8_t* output,
        size_t outputSize);

    static BoardIdentityStatus decode(
        const uint8_t* input,
        size_t inputSize,
        BoardIdentity& identity);

    static uint32_t calculateCrc32(const uint8_t* data, size_t size);
};

} // namespace EnvNode
