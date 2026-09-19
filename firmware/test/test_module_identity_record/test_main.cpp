#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "IdentityRecordCrc.h"
#include "ModuleIdentityStore.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

const uint8_t ExampleRecord[ModuleIdentityCodec::EncodedSize] = {
    0x45, 0x4D, 0x49, 0x44, 0x01, 0x20, 0x01, 0x00,
    0x00, 0x03, 0x0C, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x8D, 0xFB, 0xB8, 0x98,
};

void writeCrc(uint8_t* record) {
    const uint32_t crc = calculateIdentityRecordCrc32(record, 28);
    record[28] = static_cast<uint8_t>(crc);
    record[29] = static_cast<uint8_t>(crc >> 8);
    record[30] = static_cast<uint8_t>(crc >> 16);
    record[31] = static_cast<uint8_t>(crc >> 24);
}

class MemoryModuleStorage : public IModuleIdentityStorage {
public:
    MemoryModuleStorage() { std::memset(bytes, 0xFF, sizeof(bytes)); }

    bool read(uint16_t address, uint8_t* data, size_t size) override {
        ++readCount;
        if (failRead || static_cast<size_t>(address) + size > sizeof(bytes)) return false;
        std::memcpy(data, bytes + address, size);
        if (corruptReadback && readCount > 1) data[10] ^= 0x01;
        return true;
    }

    bool write(uint16_t address, const uint8_t* data, size_t size) override {
        ++writeCount;
        if (failWriteNumber == writeCount
            || static_cast<size_t>(address) + size > sizeof(bytes)) return false;
        writeAddresses[writeCount - 1] = address;
        writeSizes[writeCount - 1] = size;
        std::memcpy(bytes + address, data, size);
        return true;
    }

    uint8_t bytes[256];
    uint16_t writeAddresses[3] = {};
    size_t writeSizes[3] = {};
    size_t readCount = 0;
    size_t writeCount = 0;
    size_t failWriteNumber = 0;
    bool failRead = false;
    bool corruptReadback = false;
};

const ModuleIdentity DuoRelayIdentity = {
    ModuleProfileId::DuoRelay,
    {0, 3},
    12,
};

} // namespace

void test_documented_record_round_trips() {
    ModuleIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::Valid),
        static_cast<int>(ModuleIdentityCodec::decode(
            ExampleRecord, sizeof(ExampleRecord), identity)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProfileId::DuoRelay),
        static_cast<int>(identity.profileId));
    TEST_ASSERT_EQUAL_UINT8(3, identity.revision.minor);
    TEST_ASSERT_EQUAL_UINT32(12, identity.serialNumber);

    uint8_t encoded[ModuleIdentityCodec::EncodedSize];
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::Valid),
        static_cast<int>(ModuleIdentityCodec::encode(
            DuoRelayIdentity, encoded, sizeof(encoded))));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(ExampleRecord, encoded, sizeof(encoded));
}

void test_blank_transport_and_integrity_failures_are_distinct() {
    uint8_t record[ModuleIdentityCodec::EncodedSize];
    std::memset(record, 0xFF, sizeof(record));
    ModuleIdentity identity = {};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::NotProvisioned),
        static_cast<int>(ModuleIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[10] ^= 1;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::InvalidCRC),
        static_cast<int>(ModuleIdentityCodec::decode(record, sizeof(record), identity)));

    MemoryModuleStorage storage;
    storage.failRead = true;
    ModuleIdentityStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::StorageUnavailable),
        static_cast<int>(store.read().status));
}

void test_unknown_profile_revision_and_serial_are_distinct() {
    uint8_t record[ModuleIdentityCodec::EncodedSize];
    ModuleIdentity identity = {};

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[6] = 0xFF;
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::UnknownModuleProfile),
        static_cast<int>(ModuleIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    record[9] = 2;
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::UnsupportedRevision),
        static_cast<int>(ModuleIdentityCodec::decode(record, sizeof(record), identity)));

    std::memcpy(record, ExampleRecord, sizeof(record));
    std::memset(record + 10, 0, 4);
    writeCrc(record);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::UnassignedSerial),
        static_cast<int>(ModuleIdentityCodec::decode(record, sizeof(record), identity)));
}

void test_store_commits_magic_last_and_verifies_readback() {
    MemoryModuleStorage storage;
    ModuleIdentityStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::Success),
        static_cast<int>(store.write(DuoRelayIdentity)));
    TEST_ASSERT_EQUAL_UINT32(3, storage.writeCount);
    TEST_ASSERT_EQUAL_UINT16(0, storage.writeAddresses[0]);
    TEST_ASSERT_EQUAL_UINT32(4, storage.writeSizes[0]);
    TEST_ASSERT_EQUAL_UINT16(4, storage.writeAddresses[1]);
    TEST_ASSERT_EQUAL_UINT32(28, storage.writeSizes[1]);
    TEST_ASSERT_EQUAL_UINT16(0, storage.writeAddresses[2]);
    TEST_ASSERT_EQUAL_UINT32(4, storage.writeSizes[2]);
    TEST_ASSERT_EQUAL_UINT32(1, storage.readCount);
}

void test_store_reports_write_and_readback_failures() {
    for (size_t failedWrite = 1; failedWrite <= 3; ++failedWrite) {
        MemoryModuleStorage storage;
        storage.failWriteNumber = failedWrite;
        ModuleIdentityStore store(storage);
        TEST_ASSERT_EQUAL_INT(
            static_cast<int>(ModuleIdentityWriteStatus::WriteFailed),
            static_cast<int>(store.write(DuoRelayIdentity)));
        TEST_ASSERT_EQUAL_UINT32(failedWrite, storage.writeCount);
    }

    MemoryModuleStorage unavailable;
    unavailable.failRead = true;
    ModuleIdentityStore unavailableStore(unavailable);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::ReadbackFailed),
        static_cast<int>(unavailableStore.write(DuoRelayIdentity)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_documented_record_round_trips);
    RUN_TEST(test_blank_transport_and_integrity_failures_are_distinct);
    RUN_TEST(test_unknown_profile_revision_and_serial_are_distinct);
    RUN_TEST(test_store_commits_magic_last_and_verifies_readback);
    RUN_TEST(test_store_reports_write_and_readback_failures);
    return UNITY_END();
}
