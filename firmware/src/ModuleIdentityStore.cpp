#include "ModuleIdentityStore.h"

#include <cstring>

namespace EnvNode {
namespace {

const uint8_t RecordAddress = 0x00;
const size_t MagicSize = 4;

bool identitiesEqual(const ModuleIdentity& left, const ModuleIdentity& right) {
    return left.profileId == right.profileId
        && left.revision.major == right.revision.major
        && left.revision.minor == right.revision.minor
        && left.serialNumber == right.serialNumber;
}

} // namespace

ModuleIdentityStore::ModuleIdentityStore(IModuleIdentityStorage& storage)
    : storage_(storage) {
}

ModuleIdentityReadResult ModuleIdentityStore::read() {
    ModuleIdentityReadResult result;
    uint8_t encoded[ModuleIdentityCodec::EncodedSize];
    if (!storage_.read(RecordAddress, encoded, sizeof(encoded))) {
        return result;
    }
    result.status = ModuleIdentityCodec::decode(
        encoded, sizeof(encoded), result.identity);
    return result;
}

ModuleIdentityWriteStatus ModuleIdentityStore::write(const ModuleIdentity& identity) {
    uint8_t encoded[ModuleIdentityCodec::EncodedSize];
    const ModuleIdentityStatus encodeStatus =
        ModuleIdentityCodec::encode(identity, encoded, sizeof(encoded));
    if (encodeStatus != ModuleIdentityStatus::Valid
        && encodeStatus != ModuleIdentityStatus::UnassignedSerial) {
        return ModuleIdentityWriteStatus::InvalidIdentity;
    }

    const uint8_t invalidMagic[MagicSize] = {};
    if (!storage_.write(RecordAddress, invalidMagic, sizeof(invalidMagic))
        || !storage_.write(
            RecordAddress + MagicSize,
            encoded + MagicSize,
            sizeof(encoded) - MagicSize)
        || !storage_.write(RecordAddress, encoded, MagicSize)) {
        return ModuleIdentityWriteStatus::WriteFailed;
    }

    uint8_t readback[ModuleIdentityCodec::EncodedSize];
    if (!storage_.read(RecordAddress, readback, sizeof(readback))) {
        return ModuleIdentityWriteStatus::ReadbackFailed;
    }
    ModuleIdentity decoded = {};
    const ModuleIdentityStatus readbackStatus =
        ModuleIdentityCodec::decode(readback, sizeof(readback), decoded);
    if (readbackStatus != encodeStatus) {
        return ModuleIdentityWriteStatus::ReadbackInvalid;
    }
    if (std::memcmp(encoded, readback, sizeof(encoded)) != 0
        || !identitiesEqual(identity, decoded)) {
        return ModuleIdentityWriteStatus::ReadbackMismatch;
    }
    return ModuleIdentityWriteStatus::Success;
}

} // namespace EnvNode
