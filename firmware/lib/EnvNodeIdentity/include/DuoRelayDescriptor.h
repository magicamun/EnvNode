#pragma once

#include <cstddef>
#include <cstdint>

#include "CompactCborCodec.h"
#include "ModuleManufacturingData.h"

namespace EnvNode {

using DuoRelayDescriptorManufacturingData = ModuleManufacturingData;

class DuoRelayDescriptor {
public:
    static constexpr size_t MaximumEncodedSize = 768;

    static CompactCborStatus encode(
        const DuoRelayDescriptorManufacturingData& manufacturing,
        uint8_t* output,
        size_t capacity,
        size_t& encodedSize);
};

} // namespace EnvNode
