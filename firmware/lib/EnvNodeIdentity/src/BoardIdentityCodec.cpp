#include "BoardIdentityCodec.h"

#include <cstring>

#include "IdentityRecordCrc.h"

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
    const size_t serialLength = std::strlen(identity.serialNumber.value);
    output[10] = static_cast<uint8_t>(serialLength);
    std::memcpy(output + 11, identity.serialNumber.value, serialLength);
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
    if (input[4] != FormatVersion && input[4] != LegacyNumericFormatVersion) {
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
    std::memset(identity.serialNumber.value, 0, sizeof(identity.serialNumber.value));
    if (input[4] == LegacyNumericFormatVersion) {
        uint32_t numericSerial = readUint32LittleEndian(input + 10);
        if (numericSerial != 0) {
            char reversed[10];
            size_t length = 0;
            while (numericSerial != 0) {
                reversed[length++] = static_cast<char>('0' + numericSerial % 10);
                numericSerial /= 10;
            }
            for (size_t index = 0; index < length; ++index) {
                identity.serialNumber.value[index] = reversed[length - index - 1];
            }
        }
    } else {
        const size_t serialLength = input[10];
        if (serialLength > MaxBoardSerialNumberLength) {
            return BoardIdentityStatus::InvalidSerial;
        }
        std::memcpy(identity.serialNumber.value, input + 11, serialLength);
    }
    return validateBoardIdentity(identity);
}

uint32_t BoardIdentityCodec::calculateCrc32(const uint8_t* data, size_t size) {
    return calculateIdentityRecordCrc32(data, size);
}

} // namespace EnvNode
