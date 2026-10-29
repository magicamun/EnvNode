#pragma once

#include <cstddef>
#include <cstdint>

#include "BoardIdentity.h"

namespace EnvNode {

class BoardIdentityCodec {
public:
    static constexpr size_t EncodedSize = 32;
    static constexpr uint8_t FormatVersion = 2;
    static constexpr uint8_t LegacyNumericFormatVersion = 1;

    static BoardIdentityStatus encode(
        const BoardIdentity& identity,
        uint8_t* output,
        size_t outputSize, bool requireKnownProfile = true);

    static BoardIdentityStatus decode(
        const uint8_t* input,
        size_t inputSize,
        BoardIdentity& identity, bool requireKnownProfile = true);

    static uint32_t calculateCrc32(const uint8_t* data, size_t size);
};

} // namespace EnvNode
