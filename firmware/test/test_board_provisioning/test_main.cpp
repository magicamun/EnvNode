#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "BoardProvisioningService.h"
#include "BoardProfile.h"

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
    bool read(uint16_t address, uint8_t* data, size_t size) override {
        if (failRead) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }
    bool write(uint16_t address, const uint8_t* data, size_t size) override {
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

const BoardIdentity Identity = {BoardProfileId::EnvNodeMainboard, {0, 3}, {"MB-12"}};

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
    TEST_ASSERT_EQUAL_STRING("MB-12", readback.identity.serialNumber.value);
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

void test_weather_provisioning_round_trip_and_profile() {
    MemoryStorage storage;
    BoardIdentityStore store(storage);
    BoardProvisioningService service(store);
    const BoardIdentity weather = {BoardProfileId::EnvNodeWeather, {0, 2}, {"WS-42"}};
    const auto result = service.provision(weather, true);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardProvisioningStatus::Success),
        static_cast<int>(result.status));
    TEST_ASSERT_TRUE(result.rebootRequired);
    const auto read = store.read();
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(read.status));
    TEST_ASSERT_EQUAL_STRING("WS-42", read.identity.serialNumber.value);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardProfileId::EnvNodeWeather),
        static_cast<int>(read.identity.profileId));
    const auto* profile = boardProfile(read.identity.profileId);
    TEST_ASSERT_NOT_NULL(profile);
    TEST_ASSERT_EQUAL_UINT8(2, profile->revision.minor);
    TEST_ASSERT_EQUAL_UINT32(1, profile->moduleSlotCount);
    TEST_ASSERT_NULL(boardModuleSlot(*profile, ModuleSlot::B));
    const auto* slot = boardModuleSlot(*profile, ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(slot);
    TEST_ASSERT_EQUAL_UINT8(0x52, slot->identityEepromAddress);
    TEST_ASSERT_EQUAL_UINT8(33, slot->auxGpio1.number);
    TEST_ASSERT_EQUAL_UINT8(13, slot->auxGpio2.number);
    TEST_ASSERT_EQUAL_UINT8(5, slot->spiChipSelect.number);
    TEST_ASSERT_TRUE(selectCurrentBoardProfile(profile->id));
    TEST_ASSERT_TRUE(hasGpioCapabilities(
        BoardCapabilities::current().gpio(GpioResource(27))->capabilities,
        GpioCapability::DigitalOutput));
    TEST_ASSERT_NULL(BoardCapabilities::current().gpio(GpioResource(15)));
    TEST_ASSERT_FALSE(hasGpioCapabilities(
        BoardCapabilities::current().gpio(GpioResource(34))->capabilities,
        GpioCapability::DigitalOutput));
    BoardIdentity invalid = weather;
    invalid.revision.minor = 3;
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardProvisioningStatus::InvalidIdentity),
        static_cast<int>(service.provision(invalid, true).status));
}

void test_mini_08_profile_has_expected_slots_and_adc_inputs() {
    const BoardProfile* profile = boardProfile(BoardProfileId::EnvNodeMini);
    TEST_ASSERT_NOT_NULL(profile);
    TEST_ASSERT_EQUAL_UINT8(0, profile->revision.major);
    TEST_ASSERT_EQUAL_UINT8(8, profile->revision.minor);
    TEST_ASSERT_EQUAL_UINT32(2, profile->moduleSlotCount);
    const auto* slotA = boardModuleSlot(*profile, ModuleSlot::A);
    const auto* slotB = boardModuleSlot(*profile, ModuleSlot::B);
    TEST_ASSERT_NOT_NULL(slotA);
    TEST_ASSERT_NOT_NULL(slotB);
    TEST_ASSERT_EQUAL_UINT8(33, slotA->auxGpio1.number);
    TEST_ASSERT_EQUAL_UINT8(13, slotA->auxGpio2.number);
    TEST_ASSERT_EQUAL_UINT8(5, slotA->spiChipSelect.number);
    TEST_ASSERT_EQUAL_UINT8(32, slotB->auxGpio1.number);
    TEST_ASSERT_EQUAL_UINT8(14, slotB->auxGpio2.number);
    TEST_ASSERT_EQUAL_UINT8(27, slotB->spiChipSelect.number);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardIdentityStatus::Valid),
        static_cast<int>(validateBoardIdentity(
            {BoardProfileId::EnvNodeMini, {0, 8}, {"MINI-A01"}})));
    TEST_ASSERT_EQUAL_INT(static_cast<int>(BoardIdentityStatus::UnsupportedRevision),
        static_cast<int>(validateBoardIdentity(
            {BoardProfileId::EnvNodeMini, {0, 9}, {"MINI-A01"}})));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_confirmation_is_required_before_any_write);
    RUN_TEST(test_invalid_identity_is_rejected_before_storage);
    RUN_TEST(test_success_requires_verified_store_write_and_requests_reboot);
    RUN_TEST(test_missing_hardware_is_a_controlled_write_failure);
    RUN_TEST(test_readback_failure_does_not_request_reboot);
    RUN_TEST(test_weather_provisioning_round_trip_and_profile);
    RUN_TEST(test_mini_08_profile_has_expected_slots_and_adc_inputs);
    return UNITY_END();
}
