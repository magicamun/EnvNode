#pragma once

#include <cstddef>
#include <cstdint>

#include "ModuleIdentity.h"

namespace EnvNode {

class ModuleIdentityCodec {
public:
    // Legacy fixed-size EMID version 1. Kept for current hardware provisioning
    // and migration into the future self-describing descriptor format.
    static constexpr size_t EncodedSize = 32;
    static constexpr uint8_t FormatVersion = 1;

    static ModuleIdentityStatus encode(
        const ModuleIdentity& identity,
        uint8_t* output,
        size_t outputSize);
    static ModuleIdentityStatus decode(
        const uint8_t* input,
        size_t inputSize,
        ModuleIdentity& identity);
};

} // namespace EnvNode
