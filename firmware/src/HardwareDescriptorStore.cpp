#include "HardwareDescriptorStore.h"

#include <cstring>

#include "IdentityRecordCrc.h"

namespace EnvNode {
namespace {

const size_t VerificationChunkSize = 32;

bool isNewer(uint32_t candidate, uint32_t reference) {
    return static_cast<int32_t>(candidate - reference) > 0;
}

} // namespace

HardwareDescriptorStore::HardwareDescriptorStore(IIdentityStorage& storage)
    : storage_(storage) {
}

uint16_t HardwareDescriptorStore::bankAddress(HardwareDescriptorBank bank) const {
    return bank == HardwareDescriptorBank::B ? BankBAddress : BankAAddress;
}

HardwareDescriptorStore::BankInspection HardwareDescriptorStore::inspect(
    HardwareDescriptorBank bank) {
    BankInspection result;
    uint8_t encoded[HardwareDescriptorEnvelope::EncodedSize];
    const uint16_t address = bankAddress(bank);
    if (!storage_.read(address, encoded, sizeof(encoded))) {
        result.storageAvailable = false;
        return result;
    }

    result.hasMagic = std::memcmp(encoded, "ENHD", 4) == 0;
    if (HardwareDescriptorEnvelopeCodec::decode(
            encoded, sizeof(encoded), MaximumPayloadSize, result.envelope)
        != HardwareDescriptorEnvelopeStatus::Valid) {
        return result;
    }

    IdentityRecordCrc32 crc;
    uint8_t chunk[VerificationChunkSize];
    size_t offset = 0;
    while (offset < result.envelope.payloadLength) {
        const size_t remaining = result.envelope.payloadLength - offset;
        const size_t chunkSize = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        if (!storage_.read(
                static_cast<uint16_t>(address + HardwareDescriptorEnvelope::EncodedSize + offset),
                chunk, chunkSize)) {
            result.storageAvailable = false;
            return result;
        }
        crc.update(chunk, chunkSize);
        offset += chunkSize;
    }
    result.valid = crc.value() == result.envelope.payloadCrc32;
    return result;
}

HardwareDescriptorReadResult HardwareDescriptorStore::read(
    uint8_t* payload,
    size_t capacity) {
    HardwareDescriptorReadResult result;
    const BankInspection a = inspect(HardwareDescriptorBank::A);
    const BankInspection b = inspect(HardwareDescriptorBank::B);
    if (!a.storageAvailable || !b.storageAvailable) {
        result.status = HardwareDescriptorStoreStatus::StorageUnavailable;
        return result;
    }
    if (!a.valid && !b.valid) {
        result.status = (!a.hasMagic && !b.hasMagic)
            ? HardwareDescriptorStoreStatus::NotProvisioned
            : HardwareDescriptorStoreStatus::NoValidBank;
        return result;
    }

    const bool selectB = b.valid && (!a.valid
        || isNewer(b.envelope.generation, a.envelope.generation));
    result.bank = selectB ? HardwareDescriptorBank::B : HardwareDescriptorBank::A;
    result.envelope = selectB ? b.envelope : a.envelope;
    if (payload == nullptr || capacity < result.envelope.payloadLength) {
        result.status = HardwareDescriptorStoreStatus::BufferTooSmall;
        return result;
    }
    if (!storage_.read(
            static_cast<uint16_t>(bankAddress(result.bank)
                + HardwareDescriptorEnvelope::EncodedSize),
            payload, result.envelope.payloadLength)) {
        result.status = HardwareDescriptorStoreStatus::StorageUnavailable;
        return result;
    }
    result.status = HardwareDescriptorStoreStatus::Valid;
    return result;
}

bool HardwareDescriptorStore::payloadMatches(
    HardwareDescriptorBank bank,
    const uint8_t* expected,
    size_t size) {
    uint8_t chunk[VerificationChunkSize];
    size_t offset = 0;
    while (offset < size) {
        const size_t remaining = size - offset;
        const size_t chunkSize = remaining < sizeof(chunk) ? remaining : sizeof(chunk);
        if (!storage_.read(
                static_cast<uint16_t>(bankAddress(bank)
                    + HardwareDescriptorEnvelope::EncodedSize + offset),
                chunk, chunkSize)
            || std::memcmp(chunk, expected + offset, chunkSize) != 0) {
            return false;
        }
        offset += chunkSize;
    }
    return true;
}

HardwareDescriptorWriteResult HardwareDescriptorStore::write(
    HardwareDescriptorObjectKind objectKind,
    const uint8_t* payload,
    size_t payloadSize) {
    HardwareDescriptorWriteResult result;
    if (payload == nullptr || payloadSize == 0 || payloadSize > MaximumPayloadSize
        || (objectKind != HardwareDescriptorObjectKind::Board
            && objectKind != HardwareDescriptorObjectKind::Module)) {
        return result;
    }

    const BankInspection a = inspect(HardwareDescriptorBank::A);
    const BankInspection b = inspect(HardwareDescriptorBank::B);
    if (!a.storageAvailable || !b.storageAvailable) {
        result.status = HardwareDescriptorStoreStatus::StorageUnavailable;
        return result;
    }

    uint32_t generation = 1;
    HardwareDescriptorBank target = HardwareDescriptorBank::B;
    if (a.valid && b.valid) {
        const bool bNewer = isNewer(b.envelope.generation, a.envelope.generation);
        generation = (bNewer ? b.envelope.generation : a.envelope.generation) + 1U;
        target = bNewer ? HardwareDescriptorBank::A : HardwareDescriptorBank::B;
    } else if (a.valid) {
        generation = a.envelope.generation + 1U;
        target = HardwareDescriptorBank::B;
    } else if (b.valid) {
        generation = b.envelope.generation + 1U;
        target = HardwareDescriptorBank::A;
    }

    HardwareDescriptorEnvelope envelope;
    envelope.objectKind = objectKind;
    envelope.generation = generation;
    envelope.payloadLength = static_cast<uint16_t>(payloadSize);
    envelope.payloadCrc32 = calculateIdentityRecordCrc32(payload, payloadSize);
    uint8_t encoded[HardwareDescriptorEnvelope::EncodedSize];
    if (HardwareDescriptorEnvelopeCodec::encode(
            envelope, encoded, sizeof(encoded), MaximumPayloadSize)
        != HardwareDescriptorEnvelopeStatus::Valid) {
        return result;
    }

    const uint16_t address = bankAddress(target);
    const uint8_t invalidMagic[4] = {};
    if (!storage_.write(address, invalidMagic, sizeof(invalidMagic))
        || !storage_.write(
            static_cast<uint16_t>(address + HardwareDescriptorEnvelope::EncodedSize),
            payload, payloadSize)
        || !storage_.write(address + 4, encoded + 4, sizeof(encoded) - 4)
        || !storage_.write(address, encoded, 4)) {
        result.status = HardwareDescriptorStoreStatus::WriteFailed;
        return result;
    }

    const BankInspection verified = inspect(target);
    if (!verified.storageAvailable || !verified.valid
        || verified.envelope.objectKind != envelope.objectKind
        || verified.envelope.generation != envelope.generation
        || verified.envelope.payloadLength != envelope.payloadLength
        || verified.envelope.payloadCrc32 != envelope.payloadCrc32
        || !payloadMatches(target, payload, payloadSize)) {
        result.status = HardwareDescriptorStoreStatus::VerificationFailed;
        return result;
    }

    result.status = HardwareDescriptorStoreStatus::Valid;
    result.bank = target;
    result.generation = generation;
    return result;
}

const char* hardwareDescriptorStoreStatusName(HardwareDescriptorStoreStatus status) {
    switch (status) {
        case HardwareDescriptorStoreStatus::Valid: return "Valid";
        case HardwareDescriptorStoreStatus::NotProvisioned: return "NotProvisioned";
        case HardwareDescriptorStoreStatus::StorageUnavailable: return "StorageUnavailable";
        case HardwareDescriptorStoreStatus::NoValidBank: return "NoValidBank";
        case HardwareDescriptorStoreStatus::BufferTooSmall: return "BufferTooSmall";
        case HardwareDescriptorStoreStatus::InvalidArgument: return "InvalidArgument";
        case HardwareDescriptorStoreStatus::WriteFailed: return "WriteFailed";
        case HardwareDescriptorStoreStatus::VerificationFailed: return "VerificationFailed";
        default: return "InvalidArgument";
    }
}

} // namespace EnvNode
