#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "ModuleProvisioningService.h"

using namespace EnvNode;

HardwareSerial Serial;
void HardwareSerial::begin(unsigned long) {}
size_t HardwareSerial::write(const uint8_t*, size_t length) { return length; }
void pinMode(unsigned char, int) {}
void digitalWrite(unsigned char, int) {}

namespace {

class MemoryStorage : public IModuleIdentityStorage {
public:
    MemoryStorage() { std::memset(bytes, 0xFF, sizeof(bytes)); }

    bool read(uint16_t address, uint8_t* data, size_t size) override {
        if (failRead || address + size > sizeof(bytes)) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }

    bool write(uint16_t address, const uint8_t* data, size_t size) override {
        ++writes;
        if (failWrite || address + size > sizeof(bytes)) return false;
        std::memcpy(bytes + address, data, size);
        return true;
    }

    uint8_t bytes[ModuleIdentityCodec::EncodedSize];
    size_t writes = 0;
    bool failRead = false;
    bool failWrite = false;
};

const ModuleIdentity DuoRelayIdentity = {
    ModuleProfileId::DuoRelay, {0, 3}, 42,
};

struct Fixture {
    MemoryStorage slotAStorage;
    MemoryStorage slotBStorage;
    ModuleIdentityStore slotAStore{slotAStorage};
    ModuleIdentityStore slotBStore{slotBStorage};
    ModuleDiscoveryService discovery{slotAStore, slotBStore};
    ModuleProvisioningService service{slotAStore, slotBStore, discovery};
};

} // namespace

void test_confirmation_is_required_before_any_write() {
    Fixture fixture;
    const ModuleProvisioningResult result = fixture.service.provision(
        ModuleSlot::A, DuoRelayIdentity, false);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::ConfirmationRequired),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writes);
}

void test_invalid_slot_is_rejected_before_any_write() {
    Fixture fixture;
    const ModuleProvisioningResult result = fixture.service.provision(
        static_cast<ModuleSlot>(2), DuoRelayIdentity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::InvalidSlot),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writes);
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotBStorage.writes);
}

void test_invalid_identity_is_rejected_before_storage() {
    Fixture fixture;
    ModuleIdentity invalid = DuoRelayIdentity;
    invalid.revision.minor = 4;
    const ModuleProvisioningResult result = fixture.service.provision(
        ModuleSlot::A, invalid, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::InvalidIdentity),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writes);
}

void test_success_writes_selected_slot_and_refreshes_discovery() {
    Fixture fixture;
    const ModuleProvisioningResult result = fixture.service.provision(
        ModuleSlot::B, DuoRelayIdentity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::Success),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writes);
    // The store commits atomically by invalidating the magic, writing the
    // payload, and restoring the magic as the final step.
    TEST_ASSERT_EQUAL_UINT32(3, fixture.slotBStorage.writes);

    const ModuleDiscoveryResult* discovered = fixture.discovery.result(ModuleSlot::B);
    TEST_ASSERT_NOT_NULL(discovered);
    TEST_ASSERT_TRUE(discovered->identified());
    TEST_ASSERT_EQUAL_UINT32(42, discovered->identity.serialNumber);
    TEST_ASSERT_NOT_NULL(discovered->profile);
}

void test_missing_module_is_reported_as_write_failure() {
    Fixture fixture;
    fixture.slotAStorage.failWrite = true;
    const ModuleProvisioningResult result = fixture.service.provision(
        ModuleSlot::A, DuoRelayIdentity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::WriteFailed),
        static_cast<int>(result.status));
}

void test_readback_failure_is_reported() {
    Fixture fixture;
    fixture.slotAStorage.failRead = true;
    const ModuleProvisioningResult result = fixture.service.provision(
        ModuleSlot::A, DuoRelayIdentity, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProvisioningStatus::ReadbackFailed),
        static_cast<int>(result.status));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_confirmation_is_required_before_any_write);
    RUN_TEST(test_invalid_slot_is_rejected_before_any_write);
    RUN_TEST(test_invalid_identity_is_rejected_before_storage);
    RUN_TEST(test_success_writes_selected_slot_and_refreshes_discovery);
    RUN_TEST(test_missing_module_is_reported_as_write_failure);
    RUN_TEST(test_readback_failure_is_reported);
    return UNITY_END();
}
