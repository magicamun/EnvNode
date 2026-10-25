#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

enum class HardwareDescriptorObjectKind : uint8_t {
    Board = 0,
    Module = 1,
};

enum class HardwareDescriptorEnvelopeStatus : uint8_t {
    Valid,
    NotDescriptor,
    UnsupportedEnvelopeVersion,
    UnsupportedEncoding,
    InvalidObjectKind,
    UnsupportedRecordKind,
    UnsupportedSchema,
    InvalidFlags,
    InvalidPayloadLength,
    InvalidHeaderCrc,
};

struct HardwareDescriptorEnvelope {
    static constexpr size_t EncodedSize = 32;
    static constexpr uint8_t CurrentEnvelopeVersion = 1;
    static constexpr uint8_t CompactCborEncoding = 1;
    static constexpr uint8_t CompleteDescriptorRecord = 0;
    static constexpr uint16_t CurrentSchemaMajor = 0;
    static constexpr uint16_t CurrentSchemaMinor = 1;

    HardwareDescriptorObjectKind objectKind = HardwareDescriptorObjectKind::Board;
    uint32_t generation = 0;
    uint16_t payloadLength = 0;
    uint32_t payloadCrc32 = 0;
};

class HardwareDescriptorEnvelopeCodec {
public:
    static HardwareDescriptorEnvelopeStatus encode(
        const HardwareDescriptorEnvelope& envelope,
        uint8_t* output,
        size_t outputSize,
        uint16_t maximumPayloadLength);

    static HardwareDescriptorEnvelopeStatus decode(
        const uint8_t* input,
        size_t inputSize,
        uint16_t maximumPayloadLength,
        HardwareDescriptorEnvelope& envelope);
};

const char* hardwareDescriptorEnvelopeStatusName(
    HardwareDescriptorEnvelopeStatus status);

} // namespace EnvNode
