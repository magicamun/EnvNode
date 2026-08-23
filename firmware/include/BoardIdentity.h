#pragma once

#include <cstdint>

#include "BoardProfile.h"

namespace EnvNode {

using BoardSerialNumber = uint32_t;

struct BoardIdentity {
    BoardProfileId profileId;
    BoardRevision revision;
    BoardSerialNumber serialNumber;
};

enum class BoardIdentityStatus : uint8_t {
    Valid,
    StorageUnavailable,
    NotProvisioned,
    InvalidMagic,
    UnsupportedFormat,
    InvalidLength,
    InvalidCRC,
    UnknownBoardProfile,
    InvalidRevision,
    UnsupportedRevision,
    UnassignedSerial,
};

bool isKnownBoardProfileId(uint16_t encodedId);
bool decodeBoardProfileId(uint16_t encodedId, BoardProfileId& profileId);
uint16_t encodeBoardProfileId(BoardProfileId profileId);
BoardIdentityStatus validateBoardIdentity(const BoardIdentity& identity);

} // namespace EnvNode
