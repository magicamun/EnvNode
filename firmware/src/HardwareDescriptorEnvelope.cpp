#include "HardwareDescriptorEnvelope.h"

#include <cstring>

#include "IdentityRecordCrc.h"

namespace EnvNode {
namespace {

const uint8_t Magic[4] = {'E', 'N', 'H', 'D'};
const size_t HeaderCrcOffset = 24;

void writeUint16(uint8_t* target, uint16_t value) {
    target[0] = static_cast<uint8_t>(value >> 8);
    target[1] = static_cast<uint8_t>(value);
}

void writeUint32(uint8_t* target, uint32_t value) {
    target[0] = static_cast<uint8_t>(value >> 24);
    target[1] = static_cast<uint8_t>(value >> 16);
    target[2] = static_cast<uint8_t>(value >> 8);
    target[3] = static_cast<uint8_t>(value);
}

uint16_t readUint16(const uint8_t* source) {
    return static_cast<uint16_t>(source[0]) << 8
        | static_cast<uint16_t>(source[1]);
}

uint32_t readUint32(const uint8_t* source) {
    return static_cast<uint32_t>(source[0]) << 24
        | static_cast<uint32_t>(source[1]) << 16
        | static_cast<uint32_t>(source[2]) << 8
        | static_cast<uint32_t>(source[3]);
}

bool objectKindValid(HardwareDescriptorObjectKind kind) {
    return kind == HardwareDescriptorObjectKind::Board
        || kind == HardwareDescriptorObjectKind::Module;
}

uint32_t headerCrc(const uint8_t* input) {
    uint8_t copy[HardwareDescriptorEnvelope::EncodedSize];
    std::memcpy(copy, input, sizeof(copy));
    std::memset(copy + HeaderCrcOffset, 0, 4);
    return calculateIdentityRecordCrc32(copy, sizeof(copy));
}

} // namespace

HardwareDescriptorEnvelopeStatus HardwareDescriptorEnvelopeCodec::encode(
    const HardwareDescriptorEnvelope& envelope,
    uint8_t* output,
    size_t outputSize,
    uint16_t maximumPayloadLength) {
    if (output == nullptr || outputSize < HardwareDescriptorEnvelope::EncodedSize
        || envelope.payloadLength == 0
        || envelope.payloadLength > maximumPayloadLength) {
        return HardwareDescriptorEnvelopeStatus::InvalidPayloadLength;
    }
    if (!objectKindValid(envelope.objectKind)) {
        return HardwareDescriptorEnvelopeStatus::InvalidObjectKind;
    }

    std::memset(output, 0, HardwareDescriptorEnvelope::EncodedSize);
    std::memcpy(output, Magic, sizeof(Magic));
    output[4] = HardwareDescriptorEnvelope::CurrentEnvelopeVersion;
    output[5] = HardwareDescriptorEnvelope::CompactCborEncoding;
    output[6] = static_cast<uint8_t>(envelope.objectKind);
    output[7] = HardwareDescriptorEnvelope::CompleteDescriptorRecord;
    writeUint16(output + 8, HardwareDescriptorEnvelope::CurrentSchemaMajor);
    writeUint16(output + 10, HardwareDescriptorEnvelope::CurrentSchemaMinor);
    writeUint32(output + 12, envelope.generation);
    writeUint16(output + 16, envelope.payloadLength);
    writeUint32(output + 20, envelope.payloadCrc32);
    writeUint32(output + HeaderCrcOffset, headerCrc(output));
    return HardwareDescriptorEnvelopeStatus::Valid;
}

HardwareDescriptorEnvelopeStatus HardwareDescriptorEnvelopeCodec::decode(
    const uint8_t* input,
    size_t inputSize,
    uint16_t maximumPayloadLength,
    HardwareDescriptorEnvelope& envelope) {
    if (input == nullptr || inputSize < HardwareDescriptorEnvelope::EncodedSize
        || std::memcmp(input, Magic, sizeof(Magic)) != 0) {
        return HardwareDescriptorEnvelopeStatus::NotDescriptor;
    }
    if (input[4] != HardwareDescriptorEnvelope::CurrentEnvelopeVersion) {
        return HardwareDescriptorEnvelopeStatus::UnsupportedEnvelopeVersion;
    }
    if (input[5] != HardwareDescriptorEnvelope::CompactCborEncoding) {
        return HardwareDescriptorEnvelopeStatus::UnsupportedEncoding;
    }
    const HardwareDescriptorObjectKind objectKind =
        static_cast<HardwareDescriptorObjectKind>(input[6]);
    if (!objectKindValid(objectKind)) {
        return HardwareDescriptorEnvelopeStatus::InvalidObjectKind;
    }
    if (input[7] != HardwareDescriptorEnvelope::CompleteDescriptorRecord) {
        return HardwareDescriptorEnvelopeStatus::UnsupportedRecordKind;
    }
    if (readUint16(input + 8) != HardwareDescriptorEnvelope::CurrentSchemaMajor
        || readUint16(input + 10) != HardwareDescriptorEnvelope::CurrentSchemaMinor) {
        return HardwareDescriptorEnvelopeStatus::UnsupportedSchema;
    }
    if (readUint16(input + 18) != 0) {
        return HardwareDescriptorEnvelopeStatus::InvalidFlags;
    }
    const uint16_t payloadLength = readUint16(input + 16);
    if (payloadLength == 0 || payloadLength > maximumPayloadLength) {
        return HardwareDescriptorEnvelopeStatus::InvalidPayloadLength;
    }
    if (readUint32(input + HeaderCrcOffset) != headerCrc(input)) {
        return HardwareDescriptorEnvelopeStatus::InvalidHeaderCrc;
    }

    envelope.objectKind = objectKind;
    envelope.generation = readUint32(input + 12);
    envelope.payloadLength = payloadLength;
    envelope.payloadCrc32 = readUint32(input + 20);
    return HardwareDescriptorEnvelopeStatus::Valid;
}

const char* hardwareDescriptorEnvelopeStatusName(
    HardwareDescriptorEnvelopeStatus status) {
    switch (status) {
        case HardwareDescriptorEnvelopeStatus::Valid: return "Valid";
        case HardwareDescriptorEnvelopeStatus::NotDescriptor: return "NotDescriptor";
        case HardwareDescriptorEnvelopeStatus::UnsupportedEnvelopeVersion: return "UnsupportedEnvelopeVersion";
        case HardwareDescriptorEnvelopeStatus::UnsupportedEncoding: return "UnsupportedEncoding";
        case HardwareDescriptorEnvelopeStatus::InvalidObjectKind: return "InvalidObjectKind";
        case HardwareDescriptorEnvelopeStatus::UnsupportedRecordKind: return "UnsupportedRecordKind";
        case HardwareDescriptorEnvelopeStatus::UnsupportedSchema: return "UnsupportedSchema";
        case HardwareDescriptorEnvelopeStatus::InvalidFlags: return "InvalidFlags";
        case HardwareDescriptorEnvelopeStatus::InvalidPayloadLength: return "InvalidPayloadLength";
        case HardwareDescriptorEnvelopeStatus::InvalidHeaderCrc: return "InvalidHeaderCrc";
        default: return "NotDescriptor";
    }
}

} // namespace EnvNode
