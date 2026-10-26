#pragma once

#include <cstddef>
#include <cstdint>

#include "CompactCborCodec.h"

namespace EnvNode {

struct DuoRelayDescriptorManufacturingData {
    uint8_t instanceId[16] = {};
    const char* serialNumber = nullptr;
    const char* productionBatch = nullptr;
    const char* productionDate = nullptr;
};

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
