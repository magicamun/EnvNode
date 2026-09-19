#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "HardwareDescriptorStore.h"
#include "IdentityRecordCrc.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class MemoryStorage : public IIdentityStorage {
public:
    MemoryStorage() { std::memset(bytes, 0xFF, sizeof(bytes)); }

    bool read(uint16_t address, uint8_t* data, size_t size) override {
        if (failReads || address + size > sizeof(bytes)) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }

    bool write(uint16_t address, const uint8_t* data, size_t size) override {
        ++writeCalls;
        if (writeCalls == failWriteCall || address + size > sizeof(bytes)) return false;
        std::memcpy(bytes + address, data, size);
        if (corruptPayloadWrite
            && address >= HardwareDescriptorStore::BankAAddress
                + HardwareDescriptorEnvelope::EncodedSize
            && address < HardwareDescriptorStore::BankAAddress
                + HardwareDescriptorStore::BankSize) {
            bytes[address] ^= 0x01;
        }
        return true;
    }

    uint8_t bytes[4096];
    size_t writeCalls = 0;
    size_t failWriteCall = 0;
    bool failReads = false;
    bool corruptPayloadWrite = false;
};

const uint8_t PayloadOne[] = {0xA2, 0x00, 0x82, 0x00, 0x01, 0x01, 0x01};
const uint8_t PayloadTwo[] = {0xA2, 0x00, 0x82, 0x00, 0x01, 0x01, 0x00};

void assertPayloadEquals(
    HardwareDescriptorStore& store,
    const uint8_t* expected,
    size_t expectedSize,
    HardwareDescriptorBank expectedBank,
    uint32_t expectedGeneration) {
    uint8_t payload[32] = {};
    const HardwareDescriptorReadResult read = store.read(payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::Valid),
        static_cast<int>(read.status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(expectedBank), static_cast<int>(read.bank));
    TEST_ASSERT_EQUAL_UINT32(expectedGeneration, read.envelope.generation);
    TEST_ASSERT_EQUAL_UINT16(expectedSize, read.envelope.payloadLength);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, payload, expectedSize);
}

} // namespace

void test_incremental_crc_matches_existing_one_shot_crc() {
    IdentityRecordCrc32 crc;
    crc.update(PayloadOne, 3);
    crc.update(PayloadOne + 3, sizeof(PayloadOne) - 3);
    TEST_ASSERT_EQUAL_HEX32(
        calculateIdentityRecordCrc32(PayloadOne, sizeof(PayloadOne)),
        crc.value());
}

void test_envelope_round_trips_and_rejects_corruption() {
    HardwareDescriptorEnvelope source;
    source.objectKind = HardwareDescriptorObjectKind::Module;
    source.generation = 0x01020304;
    source.payloadLength = sizeof(PayloadOne);
    source.payloadCrc32 = calculateIdentityRecordCrc32(PayloadOne, sizeof(PayloadOne));
    uint8_t encoded[HardwareDescriptorEnvelope::EncodedSize];
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorEnvelopeStatus::Valid),
        static_cast<int>(HardwareDescriptorEnvelopeCodec::encode(
            source, encoded, sizeof(encoded), HardwareDescriptorStore::MaximumPayloadSize)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY("ENHD", encoded, 4);
    TEST_ASSERT_EQUAL_HEX8(1, encoded[4]);
    TEST_ASSERT_EQUAL_HEX8(1, encoded[5]);
    TEST_ASSERT_EQUAL_HEX8(1, encoded[6]);
    TEST_ASSERT_EQUAL_HEX8(0, encoded[7]);

    HardwareDescriptorEnvelope decoded;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorEnvelopeStatus::Valid),
        static_cast<int>(HardwareDescriptorEnvelopeCodec::decode(
            encoded, sizeof(encoded), HardwareDescriptorStore::MaximumPayloadSize, decoded)));
    TEST_ASSERT_EQUAL_UINT32(source.generation, decoded.generation);
    TEST_ASSERT_EQUAL_UINT16(source.payloadLength, decoded.payloadLength);
    TEST_ASSERT_EQUAL_HEX32(source.payloadCrc32, decoded.payloadCrc32);

    encoded[12] ^= 0x01;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorEnvelopeStatus::InvalidHeaderCrc),
        static_cast<int>(HardwareDescriptorEnvelopeCodec::decode(
            encoded, sizeof(encoded), HardwareDescriptorStore::MaximumPayloadSize, decoded)));
}

void test_blank_storage_is_not_provisioned() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    uint8_t payload[8];
    const HardwareDescriptorReadResult result = store.read(payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::NotProvisioned),
        static_cast<int>(result.status));
}

void test_first_write_uses_bank_b_and_preserves_legacy_record() {
    MemoryStorage storage;
    const uint8_t legacy[4] = {'E', 'M', 'I', 'D'};
    std::memcpy(storage.bytes, legacy, sizeof(legacy));
    HardwareDescriptorStore store(storage);

    const HardwareDescriptorWriteResult write = store.write(
        HardwareDescriptorObjectKind::Module, PayloadOne, sizeof(PayloadOne));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::Valid),
        static_cast<int>(write.status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorBank::B), static_cast<int>(write.bank));
    TEST_ASSERT_EQUAL_UINT32(1, write.generation);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(legacy, storage.bytes, sizeof(legacy));
    assertPayloadEquals(
        store, PayloadOne, sizeof(PayloadOne), HardwareDescriptorBank::B, 1);
}

void test_successive_writes_alternate_banks_and_generations() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::Valid),
        static_cast<int>(store.write(
            HardwareDescriptorObjectKind::Board,
            PayloadOne, sizeof(PayloadOne)).status));
    const HardwareDescriptorWriteResult second = store.write(
        HardwareDescriptorObjectKind::Board, PayloadTwo, sizeof(PayloadTwo));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareDescriptorBank::A), static_cast<int>(second.bank));
    TEST_ASSERT_EQUAL_UINT32(2, second.generation);
    assertPayloadEquals(
        store, PayloadTwo, sizeof(PayloadTwo), HardwareDescriptorBank::A, 2);

    const HardwareDescriptorWriteResult third = store.write(
        HardwareDescriptorObjectKind::Board, PayloadOne, sizeof(PayloadOne));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareDescriptorBank::B), static_cast<int>(third.bank));
    TEST_ASSERT_EQUAL_UINT32(3, third.generation);
    assertPayloadEquals(
        store, PayloadOne, sizeof(PayloadOne), HardwareDescriptorBank::B, 3);
}

void test_corrupt_newest_bank_falls_back_to_previous_valid_bank() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    store.write(HardwareDescriptorObjectKind::Module, PayloadOne, sizeof(PayloadOne));
    store.write(HardwareDescriptorObjectKind::Module, PayloadTwo, sizeof(PayloadTwo));
    storage.bytes[HardwareDescriptorStore::BankAAddress
        + HardwareDescriptorEnvelope::EncodedSize] ^= 0x01;
    assertPayloadEquals(
        store, PayloadOne, sizeof(PayloadOne), HardwareDescriptorBank::B, 1);
}

void test_generation_selection_is_wrap_safe() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    store.write(HardwareDescriptorObjectKind::Module, PayloadOne, sizeof(PayloadOne));

    uint8_t* bankB = storage.bytes + HardwareDescriptorStore::BankBAddress;
    HardwareDescriptorEnvelope envelope;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorEnvelopeStatus::Valid),
        static_cast<int>(HardwareDescriptorEnvelopeCodec::decode(
            bankB, HardwareDescriptorEnvelope::EncodedSize,
            HardwareDescriptorStore::MaximumPayloadSize, envelope)));
    envelope.generation = UINT32_MAX;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorEnvelopeStatus::Valid),
        static_cast<int>(HardwareDescriptorEnvelopeCodec::encode(
            envelope, bankB, HardwareDescriptorEnvelope::EncodedSize,
            HardwareDescriptorStore::MaximumPayloadSize)));

    const HardwareDescriptorWriteResult wrapped = store.write(
        HardwareDescriptorObjectKind::Module, PayloadTwo, sizeof(PayloadTwo));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(HardwareDescriptorBank::A), static_cast<int>(wrapped.bank));
    TEST_ASSERT_EQUAL_UINT32(0, wrapped.generation);
    assertPayloadEquals(
        store, PayloadTwo, sizeof(PayloadTwo), HardwareDescriptorBank::A, 0);
}

void test_interrupted_update_keeps_previous_bank_readable() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    store.write(HardwareDescriptorObjectKind::Module, PayloadOne, sizeof(PayloadOne));
    storage.failWriteCall = storage.writeCalls + 4;
    const HardwareDescriptorWriteResult interrupted = store.write(
        HardwareDescriptorObjectKind::Module, PayloadTwo, sizeof(PayloadTwo));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::WriteFailed),
        static_cast<int>(interrupted.status));
    assertPayloadEquals(
        store, PayloadOne, sizeof(PayloadOne), HardwareDescriptorBank::B, 1);
}

void test_arguments_capacity_and_storage_failures_are_explicit() {
    MemoryStorage storage;
    HardwareDescriptorStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::InvalidArgument),
        static_cast<int>(store.write(
            HardwareDescriptorObjectKind::Module, nullptr, 0).status));
    store.write(HardwareDescriptorObjectKind::Module, PayloadOne, sizeof(PayloadOne));
    uint8_t small[2];
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::BufferTooSmall),
        static_cast<int>(store.read(small, sizeof(small)).status));
    storage.failReads = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::StorageUnavailable),
        static_cast<int>(store.read(small, sizeof(small)).status));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_incremental_crc_matches_existing_one_shot_crc);
    RUN_TEST(test_envelope_round_trips_and_rejects_corruption);
    RUN_TEST(test_blank_storage_is_not_provisioned);
    RUN_TEST(test_first_write_uses_bank_b_and_preserves_legacy_record);
    RUN_TEST(test_successive_writes_alternate_banks_and_generations);
    RUN_TEST(test_corrupt_newest_bank_falls_back_to_previous_valid_bank);
    RUN_TEST(test_generation_selection_is_wrap_safe);
    RUN_TEST(test_interrupted_update_keeps_previous_bank_readable);
    RUN_TEST(test_arguments_capacity_and_storage_failures_are_explicit);
    return UNITY_END();
}
