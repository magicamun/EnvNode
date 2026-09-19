#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "BoardIdentityStore.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class MemoryIdentityStorage : public IBoardIdentityStorage {
public:
    MemoryIdentityStorage() {
        std::memset(bytes, 0xFF, sizeof(bytes));
    }

    bool read(uint16_t address, uint8_t* data, size_t size) override {
        ++readCount;
        if (failRead || static_cast<size_t>(address) + size > sizeof(bytes)) {
            return false;
        }
        std::memcpy(data, bytes + address, size);
        if (corruptReadback && readCount > 1) {
            data[10] ^= 0x01;
        }
        return true;
    }

    bool write(uint16_t address, const uint8_t* data, size_t size) override {
        ++writeCount;
        if (failWriteNumber == writeCount
            || static_cast<size_t>(address) + size > sizeof(bytes)) {
            return false;
        }
        writeAddresses[writeCount - 1] = address;
        writeSizes[writeCount - 1] = size;
        std::memcpy(bytes + address, data, size);
        return true;
    }

    uint8_t bytes[256];
    uint8_t writeAddresses[3] = {};
    size_t writeSizes[3] = {};
    size_t readCount = 0;
    size_t writeCount = 0;
    size_t failWriteNumber = 0;
    bool failRead = false;
    bool corruptReadback = false;
};

const BoardIdentity MainboardIdentity = {
    BoardProfileId::EnvNodeMainboard,
    {0, 3},
    12,
};

} // namespace

void test_read_classifies_blank_and_storage_failure() {
    MemoryIdentityStorage storage;
    BoardIdentityStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::NotProvisioned),
        static_cast<int>(store.read().status));

    storage.failRead = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::StorageUnavailable),
        static_cast<int>(store.read().status));
}

void test_write_invalidates_magic_writes_body_then_magic_and_reads_back() {
    MemoryIdentityStorage storage;
    BoardIdentityStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityWriteStatus::Success),
        static_cast<int>(store.write(MainboardIdentity)));
    TEST_ASSERT_EQUAL_UINT32(3, storage.writeCount);
    TEST_ASSERT_EQUAL_UINT8(0, storage.writeAddresses[0]);
    TEST_ASSERT_EQUAL_UINT32(4, storage.writeSizes[0]);
    TEST_ASSERT_EQUAL_UINT8(4, storage.writeAddresses[1]);
    TEST_ASSERT_EQUAL_UINT32(28, storage.writeSizes[1]);
    TEST_ASSERT_EQUAL_UINT8(0, storage.writeAddresses[2]);
    TEST_ASSERT_EQUAL_UINT32(4, storage.writeSizes[2]);
    TEST_ASSERT_EQUAL_UINT32(1, storage.readCount);

    const BoardIdentityReadResult result = store.read();
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(12, result.identity.serialNumber);
}

void test_write_accepts_explicitly_unassigned_serial() {
    MemoryIdentityStorage storage;
    BoardIdentityStore store(storage);
    BoardIdentity identity = MainboardIdentity;
    identity.serialNumber = 0;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityWriteStatus::Success),
        static_cast<int>(store.write(identity)));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::UnassignedSerial),
        static_cast<int>(store.read().status));
}

void test_invalid_identity_is_not_written() {
    MemoryIdentityStorage storage;
    BoardIdentityStore store(storage);
    BoardIdentity identity = MainboardIdentity;
    identity.revision.minor = 1;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityWriteStatus::InvalidIdentity),
        static_cast<int>(store.write(identity)));
    TEST_ASSERT_EQUAL_UINT32(0, storage.writeCount);
}

void test_each_write_failure_is_reported_without_continuing() {
    for (size_t failedWrite = 1; failedWrite <= 3; ++failedWrite) {
        MemoryIdentityStorage storage;
        storage.failWriteNumber = failedWrite;
        BoardIdentityStore store(storage);
        TEST_ASSERT_EQUAL_INT(
            static_cast<int>(BoardIdentityWriteStatus::WriteFailed),
            static_cast<int>(store.write(MainboardIdentity)));
        TEST_ASSERT_EQUAL_UINT32(failedWrite, storage.writeCount);
        TEST_ASSERT_EQUAL_UINT32(0, storage.readCount);
    }
}

void test_readback_failure_and_corruption_are_reported() {
    MemoryIdentityStorage unavailable;
    BoardIdentityStore unavailableStore(unavailable);
    unavailable.failRead = true;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityWriteStatus::ReadbackFailed),
        static_cast<int>(unavailableStore.write(MainboardIdentity)));

    MemoryIdentityStorage corrupt;
    BoardIdentityStore corruptStore(corrupt);
    corrupt.corruptReadback = true;
    corrupt.readCount = 1;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityWriteStatus::ReadbackInvalid),
        static_cast<int>(corruptStore.write(MainboardIdentity)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_read_classifies_blank_and_storage_failure);
    RUN_TEST(test_write_invalidates_magic_writes_body_then_magic_and_reads_back);
    RUN_TEST(test_write_accepts_explicitly_unassigned_serial);
    RUN_TEST(test_invalid_identity_is_not_written);
    RUN_TEST(test_each_write_failure_is_reported_without_continuing);
    RUN_TEST(test_readback_failure_and_corruption_are_reported);
    return UNITY_END();
}
