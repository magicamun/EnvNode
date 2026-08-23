#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "BoardProvisioningService.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class MemoryStorage : public IBoardIdentityStorage {
public:
    MemoryStorage() { std::memset(bytes, 0xFF, sizeof(bytes)); }
    bool read(uint8_t address, uint8_t* data, size_t size) override {
        if (failRead) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }
    bool write(uint8_t address, const uint8_t* data, size_t size) override {
        ++writes;
        if (failWrite) return false;
        std::memcpy(bytes + address, data, size);
        return true;
    }
    uint8_t bytes[BoardIdentityCodec::EncodedSize];
    size_t writes = 0;
    bool failRead = false;
    bool failWrite = false;
};

const BoardIdentity Identity = {BoardProfileId::EnvNodeMainboard, {0, 2}, 12};

} // namespace

void test_confirmation_is_required_before_any_write() {
    MemoryStorage storage;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    const BoardProvisioningResult result = service.provision(Identity, false);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProvisioningStatus::ConfirmationRequired),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, storage.writes);
}

void test_invalid_identity_is_rejected_before_storage() {
    MemoryStorage storage;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    BoardIdentity invalid = Identity;
    invalid.revision.minor = 1;
    const BoardProvisioningResult result = service.provision(invalid, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProvisioningStatus::InvalidIdentity),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, storage.writes);
}

void test_success_requires_verified_store_write_and_requests_reboot() {
    MemoryStorage storage;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    const BoardProvisioningResult result = service.provision(Identity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProvisioningStatus::Success),
        static_cast<int>(result.status));
    TEST_ASSERT_TRUE(result.rebootRequired);
    const BoardIdentityReadResult readback = store.read();
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(readback.status));
    TEST_ASSERT_EQUAL_UINT32(12, readback.identity.serialNumber);
}

void test_missing_hardware_is_a_controlled_write_failure() {
    MemoryStorage storage;
    storage.failWrite = true;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    const BoardProvisioningResult result = service.provision(Identity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProvisioningStatus::WriteFailed),
        static_cast<int>(result.status));
    TEST_ASSERT_FALSE(result.rebootRequired);
}

void test_readback_failure_does_not_request_reboot() {
    MemoryStorage storage;
    storage.failRead = true;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    const BoardProvisioningResult result = service.provision(Identity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(BoardProvisioningStatus::ReadbackFailed),
        static_cast<int>(result.status));
    TEST_ASSERT_FALSE(result.rebootRequired);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_confirmation_is_required_before_any_write);
    RUN_TEST(test_invalid_identity_is_rejected_before_storage);
    RUN_TEST(test_success_requires_verified_store_write_and_requests_reboot);
    RUN_TEST(test_missing_hardware_is_a_controlled_write_failure);
    RUN_TEST(test_readback_failure_does_not_request_reboot);
    return UNITY_END();
}
