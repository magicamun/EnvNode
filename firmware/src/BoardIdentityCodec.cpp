#include "BoardIdentityCodec.h"

#include <cstring>

namespace EnvNode {
namespace {

const uint8_t Magic[] = {0x45, 0x4E, 0x49, 0x44};
const size_t CrcOffset = 28;

uint16_t readUint16LittleEndian(const uint8_t* data) {
    return static_cast<uint16_t>(data[0])
        | (static_cast<uint16_t>(data[1]) << 8);
}

uint32_t readUint32LittleEndian(const uint8_t* data) {
    return static_cast<uint32_t>(data[0])
        | (static_cast<uint32_t>(data[1]) << 8)
        | (static_cast<uint32_t>(data[2]) << 16)
        | (static_cast<uint32_t>(data[3]) << 24);
}

void writeUint16LittleEndian(uint8_t* data, uint16_t value) {
    data[0] = static_cast<uint8_t>(value);
    data[1] = static_cast<uint8_t>(value >> 8);
}

void writeUint32LittleEndian(uint8_t* data, uint32_t value) {
    data[0] = static_cast<uint8_t>(value);
    data[1] = static_cast<uint8_t>(value >> 8);
    data[2] = static_cast<uint8_t>(value >> 16);
    data[3] = static_cast<uint8_t>(value >> 24);
}

bool allBytesEqual(const uint8_t* data, size_t size, uint8_t value) {
    for (size_t index = 0; index < size; ++index) {
        if (data[index] != value) {
            return false;
        }
    }
    return true;
}

} // namespace

BoardIdentityStatus BoardIdentityCodec::encode(
    const BoardIdentity& identity,
    uint8_t* output,
    size_t outputSize) {
    if (output == nullptr || outputSize != EncodedSize) {
        return BoardIdentityStatus::InvalidLength;
    }

    const BoardIdentityStatus identityStatus = validateBoardIdentity(identity);
    if (identityStatus != BoardIdentityStatus::Valid
        && identityStatus != BoardIdentityStatus::UnassignedSerial) {
        return identityStatus;
    }

    std::memset(output, 0, EncodedSize);
    std::memcpy(output, Magic, sizeof(Magic));
    output[4] = FormatVersion;
    output[5] = static_cast<uint8_t>(EncodedSize);
    writeUint16LittleEndian(output + 6, encodeBoardProfileId(identity.profileId));
    output[8] = identity.revision.major;
    output[9] = identity.revision.minor;
    writeUint32LittleEndian(output + 10, identity.serialNumber);
    writeUint32LittleEndian(output + CrcOffset, calculateCrc32(output, CrcOffset));
    return identityStatus;
}

BoardIdentityStatus BoardIdentityCodec::decode(
    const uint8_t* input,
    size_t inputSize,
    BoardIdentity& identity) {
    if (input == nullptr || inputSize != EncodedSize) {
        return BoardIdentityStatus::InvalidLength;
    }

    if (allBytesEqual(input, EncodedSize, 0xFF)
        || allBytesEqual(input, EncodedSize, 0x00)) {
        return BoardIdentityStatus::NotProvisioned;
    }

    if (std::memcmp(input, Magic, sizeof(Magic)) != 0) {
        return BoardIdentityStatus::InvalidMagic;
    }
    if (input[4] != FormatVersion) {
        return BoardIdentityStatus::UnsupportedFormat;
    }
    if (input[5] != EncodedSize) {
        return BoardIdentityStatus::InvalidLength;
    }

    const uint32_t storedCrc = readUint32LittleEndian(input + CrcOffset);
    if (storedCrc != calculateCrc32(input, CrcOffset)) {
        return BoardIdentityStatus::InvalidCRC;
    }

    const uint16_t encodedProfileId = readUint16LittleEndian(input + 6);
    if (!decodeBoardProfileId(encodedProfileId, identity.profileId)) {
        return BoardIdentityStatus::UnknownBoardProfile;
    }

    identity.revision.major = input[8];
    identity.revision.minor = input[9];
    identity.serialNumber = readUint32LittleEndian(input + 10);
    return validateBoardIdentity(identity);
}

uint32_t BoardIdentityCodec::calculateCrc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint32_t reflectedPolynomial = 0xEDB88320U;
            crc = (crc >> 1) ^ ((crc & 1U) ? reflectedPolynomial : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

} // namespace EnvNode
