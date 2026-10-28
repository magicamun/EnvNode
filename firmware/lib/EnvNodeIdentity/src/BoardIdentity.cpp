#include "BoardIdentity.h"

#include <cstring>

namespace EnvNode {

bool isKnownBoardProfileId(uint16_t encodedId) {
    return encodedId == static_cast<uint16_t>(BoardProfileId::EnvNodeMainboard)
        || encodedId == static_cast<uint16_t>(BoardProfileId::EnvNodeWeather)
        || encodedId == static_cast<uint16_t>(BoardProfileId::EnvNodeMini);
}

bool decodeBoardProfileId(uint16_t encodedId, BoardProfileId& profileId) {
    if (!isKnownBoardProfileId(encodedId)) {
        return false;
    }

    profileId = static_cast<BoardProfileId>(encodedId);
    return true;
}

uint16_t encodeBoardProfileId(BoardProfileId profileId) {
    return static_cast<uint16_t>(profileId);
}

BoardIdentityStatus validateBoardIdentity(const BoardIdentity& identity) {
    if (!isKnownBoardProfileId(encodeBoardProfileId(identity.profileId))) {
        return BoardIdentityStatus::UnknownBoardProfile;
    }

    if (identity.revision.major == 0xFF && identity.revision.minor == 0xFF) {
        return BoardIdentityStatus::InvalidRevision;
    }

    switch (identity.profileId) {
        case BoardProfileId::EnvNodeMainboard:
            if (identity.revision.major != 0 || identity.revision.minor != 3) {
                return BoardIdentityStatus::UnsupportedRevision;
            }
            break;
        case BoardProfileId::EnvNodeWeather:
            if (identity.revision.major != 0 || identity.revision.minor != 2) {
                return BoardIdentityStatus::UnsupportedRevision;
            }
            break;
        case BoardProfileId::EnvNodeMini:
            if (identity.revision.major != 0 || identity.revision.minor != 8) {
                return BoardIdentityStatus::UnsupportedRevision;
            }
            break;
        default:
            return BoardIdentityStatus::UnknownBoardProfile;
    }

    if (!isValidBoardSerialNumber(identity.serialNumber.value)) {
        return BoardIdentityStatus::InvalidSerial;
    }
    if (!isBoardSerialNumberAssigned(identity.serialNumber)) {
        return BoardIdentityStatus::UnassignedSerial;
    }

    return BoardIdentityStatus::Valid;
}

bool isValidBoardSerialNumber(const char* serialNumber) {
    if (serialNumber == nullptr) return false;
    const size_t length = std::strlen(serialNumber);
    if (length > MaxBoardSerialNumberLength) return false;
    for (size_t index = 0; index < length; ++index) {
        const char c = serialNumber[index];
        const bool alphaNumeric = (c >= 'A' && c <= 'Z')
            || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9');
        if (!alphaNumeric && c != '-' && c != '_' && c != '.') return false;
    }
    return true;
}

bool setBoardSerialNumber(BoardSerialNumber& target, const char* serialNumber) {
    if (!isValidBoardSerialNumber(serialNumber)) return false;
    std::memset(target.value, 0, sizeof(target.value));
    std::memcpy(target.value, serialNumber, std::strlen(serialNumber));
    return true;
}

bool isBoardSerialNumberAssigned(const BoardSerialNumber& serialNumber) {
    return serialNumber.value[0] != '\0';
}

const char* boardIdentityStatusName(BoardIdentityStatus status) {
    switch (status) {
        case BoardIdentityStatus::Valid: return "Valid";
        case BoardIdentityStatus::StorageUnavailable: return "StorageUnavailable";
        case BoardIdentityStatus::NotProvisioned: return "NotProvisioned";
        case BoardIdentityStatus::InvalidMagic: return "InvalidMagic";
        case BoardIdentityStatus::UnsupportedFormat: return "UnsupportedFormat";
        case BoardIdentityStatus::InvalidLength: return "InvalidLength";
        case BoardIdentityStatus::InvalidCRC: return "InvalidCRC";
        case BoardIdentityStatus::UnknownBoardProfile: return "UnknownBoardProfile";
        case BoardIdentityStatus::InvalidRevision: return "InvalidRevision";
        case BoardIdentityStatus::UnsupportedRevision: return "UnsupportedRevision";
        case BoardIdentityStatus::InvalidSerial: return "InvalidSerial";
        case BoardIdentityStatus::UnassignedSerial: return "UnassignedSerial";
        default: return "Unknown";
    }
}

} // namespace EnvNode
