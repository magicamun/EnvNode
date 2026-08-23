#include "BoardIdentityStore.h"

#include <cstring>

namespace EnvNode {
namespace {

const uint8_t RecordAddress = 0x00;
const size_t MagicSize = 4;

bool identitiesEqual(const BoardIdentity& left, const BoardIdentity& right) {
    return left.profileId == right.profileId
        && left.revision.major == right.revision.major
        && left.revision.minor == right.revision.minor
        && left.serialNumber == right.serialNumber;
}

} // namespace

BoardIdentityStore::BoardIdentityStore(IBoardIdentityStorage& storage)
    : storage_(storage) {
}

BoardIdentityReadResult BoardIdentityStore::read() {
    BoardIdentityReadResult result;
    uint8_t encoded[BoardIdentityCodec::EncodedSize];
    if (!storage_.read(RecordAddress, encoded, sizeof(encoded))) {
        return result;
    }

    result.status = BoardIdentityCodec::decode(encoded, sizeof(encoded), result.identity);
    return result;
}

BoardIdentityWriteStatus BoardIdentityStore::write(const BoardIdentity& identity) {
    uint8_t encoded[BoardIdentityCodec::EncodedSize];
    const BoardIdentityStatus encodeStatus =
        BoardIdentityCodec::encode(identity, encoded, sizeof(encoded));
    if (encodeStatus != BoardIdentityStatus::Valid
        && encodeStatus != BoardIdentityStatus::UnassignedSerial) {
        return BoardIdentityWriteStatus::InvalidIdentity;
    }

    const uint8_t invalidMagic[MagicSize] = {};
    if (!storage_.write(RecordAddress, invalidMagic, sizeof(invalidMagic))
        || !storage_.write(
            RecordAddress + MagicSize,
            encoded + MagicSize,
            sizeof(encoded) - MagicSize)
        || !storage_.write(RecordAddress, encoded, MagicSize)) {
        return BoardIdentityWriteStatus::WriteFailed;
    }

    uint8_t readback[BoardIdentityCodec::EncodedSize];
    if (!storage_.read(RecordAddress, readback, sizeof(readback))) {
        return BoardIdentityWriteStatus::ReadbackFailed;
    }

    BoardIdentity decoded = {};
    const BoardIdentityStatus readbackStatus =
        BoardIdentityCodec::decode(readback, sizeof(readback), decoded);
    if (readbackStatus != encodeStatus) {
        return BoardIdentityWriteStatus::ReadbackInvalid;
    }
    if (std::memcmp(encoded, readback, sizeof(encoded)) != 0
        || !identitiesEqual(identity, decoded)) {
        return BoardIdentityWriteStatus::ReadbackMismatch;
    }

    return BoardIdentityWriteStatus::Success;
}

} // namespace EnvNode
