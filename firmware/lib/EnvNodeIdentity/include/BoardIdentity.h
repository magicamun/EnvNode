#pragma once

#include <cstddef>
#include <cstdint>

#include "BoardIdentityTypes.h"

namespace EnvNode {

constexpr size_t MaxBoardSerialNumberLength = 17;

struct BoardSerialNumber {
    char value[MaxBoardSerialNumberLength + 1];
};

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
    InvalidSerial,
    UnassignedSerial,
};

const char* boardIdentityStatusName(BoardIdentityStatus status);

bool isKnownBoardProfileId(uint16_t encodedId);
bool decodeBoardProfileId(uint16_t encodedId, BoardProfileId& profileId);
uint16_t encodeBoardProfileId(BoardProfileId profileId);
BoardIdentityStatus validateBoardIdentityStructure(const BoardIdentity& identity);
BoardIdentityStatus validateBoardIdentity(const BoardIdentity& identity);
bool isValidBoardSerialNumber(const char* serialNumber);
bool setBoardSerialNumber(BoardSerialNumber& target, const char* serialNumber);
bool isBoardSerialNumberAssigned(const BoardSerialNumber& serialNumber);

} // namespace EnvNode
