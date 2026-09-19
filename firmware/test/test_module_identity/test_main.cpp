#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "IdentityRecordCrc.h"
#include "ModuleCompatibility.h"
#include "ModuleDiscoveryService.h"
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

void test_both_registered_module_profiles_are_supported() {
    ModuleIdentity analog = {
        ModuleProfileId::AnalogHydroPressure,
        {0, 3},
        21,
    };
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::Valid),
        static_cast<int>(validateModuleIdentity(analog)));
    TEST_ASSERT_EQUAL_UINT16(2, encodeModuleProfileId(analog.profileId));
}

void test_module_profile_registry_describes_capabilities_and_resources() {
    TEST_ASSERT_EQUAL_UINT32(2, ModuleProfileRegistry::count());

    const ModuleProfile* relay = ModuleProfileRegistry::find(
        ModuleProfileId::DuoRelay, {0, 3});
    TEST_ASSERT_NOT_NULL(relay);
    TEST_ASSERT_EQUAL_STRING("duo_relay", relay->stableId);
    TEST_ASSERT_TRUE(hasModuleCapability(
        relay->capabilities, ModuleCapability::OnOffOutputs));
    TEST_ASSERT_EQUAL_UINT32(3, relay->requirementCount);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleConnectorResource::AuxGpio1),
        static_cast<int>(relay->requirements[0].resource));
    TEST_ASSERT_TRUE(hasGpioCapabilities(
        relay->requirements[0].requiredGpioCapabilities,
        GpioCapability::DigitalOutput));

    const ModuleProfile* analog = ModuleProfileRegistry::findByStableId(
        "analog_hydro_pressure");
    TEST_ASSERT_NOT_NULL(analog);
    TEST_ASSERT_TRUE(hasModuleCapability(
        analog->capabilities, ModuleCapability::AnalogCurrentInput));
    TEST_ASSERT_TRUE(hasModuleCapability(
        analog->capabilities, ModuleCapability::LocalProbeSupply));
    TEST_ASSERT_EQUAL_UINT32(2, analog->requirementCount);
    TEST_ASSERT_NULL(ModuleProfileRegistry::find(
        ModuleProfileId::DuoRelay, {0, 2}));
}

void test_current_board_maps_both_module_slots() {
    const BoardProfile& board = currentBoardProfile();
    TEST_ASSERT_EQUAL_UINT32(2, board.moduleSlotCount);

    const BoardModuleSlotCapability* slotA = boardModuleSlot(board, ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(slotA);
    TEST_ASSERT_EQUAL_HEX8(ModuleDiscoveryService::SlotAEepromAddress,
        slotA->identityEepromAddress);
    TEST_ASSERT_EQUAL_UINT8(4, slotA->auxGpio1.number);
    TEST_ASSERT_EQUAL_UINT8(13, slotA->auxGpio2.number);
    TEST_ASSERT_EQUAL_UINT8(5, slotA->spiChipSelect.number);
    TEST_ASSERT_TRUE(slotA->fiveVoltSupplyAvailable);

    const BoardModuleSlotCapability* slotB = boardModuleSlot(board, ModuleSlot::B);
    TEST_ASSERT_NOT_NULL(slotB);
    TEST_ASSERT_EQUAL_HEX8(ModuleDiscoveryService::SlotBEepromAddress,
        slotB->identityEepromAddress);
    TEST_ASSERT_EQUAL_UINT8(14, slotB->auxGpio1.number);
    TEST_ASSERT_EQUAL_UINT8(16, slotB->auxGpio2.number);
    TEST_ASSERT_EQUAL_UINT8(27, slotB->spiChipSelect.number);
    TEST_ASSERT_TRUE(slotB->fiveVoltSupplyAvailable);
}

void test_registered_modules_are_compatible_with_both_current_slots() {
    const BoardProfile& board = currentBoardProfile();
    for (size_t profileIndex = 0;
         profileIndex < ModuleProfileRegistry::count();
         ++profileIndex) {
        const ModuleProfile* module = ModuleProfileRegistry::at(profileIndex);
        TEST_ASSERT_NOT_NULL(module);
        for (size_t slotIndex = 0; slotIndex < 2; ++slotIndex) {
            const ModuleCompatibilityResult result = evaluateModuleCompatibility(
                board, static_cast<ModuleSlot>(slotIndex), module);
            TEST_ASSERT_EQUAL_INT(
                static_cast<int>(ModuleCompatibilityStatus::Compatible),
                static_cast<int>(result.status));
            TEST_ASSERT_NOT_NULL(result.slot);
        }
    }
}

void test_compatibility_reports_missing_profile_slot_and_resources() {
    const BoardGpioCapability outputOnlyGpio[] = {
        {GpioResource(4), "GPIO4", GpioCapability::DigitalOutput},
        {GpioResource(13), "GPIO13", GpioCapability::DigitalInput},
    };
    const BoardModuleSlotCapability slotA[] = {
        {ModuleSlot::A, 0x52, GpioResource(4), GpioResource(13), GpioResource(5), false},
    };
    const BoardProfile limitedBoard = {
        BoardProfileId::EnvNodeMainboard,
        "Limited test board",
        {0, 3},
        outputOnlyGpio,
        2,
        nullptr,
        0,
        slotA,
        1,
    };
    const ModuleProfile* relay = ModuleProfileRegistry::find(
        ModuleProfileId::DuoRelay, {0, 3});
    const ModuleProfile* analog = ModuleProfileRegistry::find(
        ModuleProfileId::AnalogHydroPressure, {0, 3});

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleCompatibilityStatus::ProfileUnavailable),
        static_cast<int>(evaluateModuleCompatibility(
            limitedBoard, ModuleSlot::A, nullptr).status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleCompatibilityStatus::SlotUnavailable),
        static_cast<int>(evaluateModuleCompatibility(
            limitedBoard, ModuleSlot::B, relay).status));

    const ModuleCompatibilityResult gpioMismatch = evaluateModuleCompatibility(
        limitedBoard, ModuleSlot::A, relay);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleCompatibilityStatus::CapabilityMismatch),
        static_cast<int>(gpioMismatch.status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleConnectorResource::AuxGpio2),
        static_cast<int>(gpioMismatch.failedResource));

    const ModuleCompatibilityResult missingI2C = evaluateModuleCompatibility(
        limitedBoard, ModuleSlot::A, analog);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleCompatibilityStatus::ResourceUnavailable),
        static_cast<int>(missingI2C.status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleConnectorResource::I2C0),
        static_cast<int>(missingI2C.failedResource));
}

void test_store_commits_magic_last_and_verifies_readback() {
    MemoryModuleStorage storage;
    ModuleIdentityStore store(storage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::Success),
        static_cast<int>(store.write(DuoRelayIdentity)));
    TEST_ASSERT_EQUAL_UINT32(3, storage.writeCount);
    TEST_ASSERT_EQUAL_UINT8(0, storage.writeAddresses[0]);
    TEST_ASSERT_EQUAL_UINT32(4, storage.writeSizes[0]);
    TEST_ASSERT_EQUAL_UINT8(4, storage.writeAddresses[1]);
    TEST_ASSERT_EQUAL_UINT32(28, storage.writeSizes[1]);
    TEST_ASSERT_EQUAL_UINT8(0, storage.writeAddresses[2]);
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

void test_discovery_reads_slots_independently_and_preserves_generic_fallback() {
    MemoryModuleStorage slotAStorage;
    MemoryModuleStorage slotBStorage;
    ModuleIdentityStore slotAStore(slotAStorage);
    ModuleIdentityStore slotBStore(slotBStorage);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::Success),
        static_cast<int>(slotAStore.write(DuoRelayIdentity)));
    slotBStorage.failRead = true;

    ModuleDiscoveryService discovery(slotAStore, slotBStore);
    discovery.scan();

    const ModuleDiscoveryResult* slotA = discovery.result(ModuleSlot::A);
    const ModuleDiscoveryResult* slotB = discovery.result(ModuleSlot::B);
    TEST_ASSERT_NOT_NULL(slotA);
    TEST_ASSERT_NOT_NULL(slotB);
    TEST_ASSERT_TRUE(slotA->identified());
    TEST_ASSERT_EQUAL_HEX8(0x52, slotA->eepromAddress);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleProfileId::DuoRelay),
        static_cast<int>(slotA->identity.profileId));
    TEST_ASSERT_NOT_NULL(slotA->profile);
    TEST_ASSERT_EQUAL_STRING("duo_relay", slotA->profile->stableId);
    TEST_ASSERT_FALSE(slotB->identified());
    TEST_ASSERT_NULL(slotB->profile);
    TEST_ASSERT_EQUAL_HEX8(0x53, slotB->eepromAddress);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::StorageUnavailable),
        static_cast<int>(slotB->status));
}

void test_discovery_can_rescan_without_retaining_stale_identity() {
    MemoryModuleStorage slotAStorage;
    MemoryModuleStorage slotBStorage;
    ModuleIdentityStore slotAStore(slotAStorage);
    ModuleIdentityStore slotBStore(slotBStorage);
    ModuleDiscoveryService discovery(slotAStore, slotBStore);

    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::Success),
        static_cast<int>(slotBStore.write(DuoRelayIdentity)));
    discovery.scan();
    TEST_ASSERT_TRUE(discovery.result(ModuleSlot::B)->identified());

    std::memset(slotBStorage.bytes, 0xFF, sizeof(slotBStorage.bytes));
    discovery.scan();
    TEST_ASSERT_FALSE(discovery.result(ModuleSlot::B)->identified());
    TEST_ASSERT_NULL(discovery.result(ModuleSlot::B)->profile);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityStatus::NotProvisioned),
        static_cast<int>(discovery.result(ModuleSlot::B)->status));
    TEST_ASSERT_NULL(discovery.result(static_cast<ModuleSlot>(99)));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_documented_record_round_trips);
    RUN_TEST(test_blank_transport_and_integrity_failures_are_distinct);
    RUN_TEST(test_unknown_profile_revision_and_serial_are_distinct);
    RUN_TEST(test_both_registered_module_profiles_are_supported);
    RUN_TEST(test_module_profile_registry_describes_capabilities_and_resources);
    RUN_TEST(test_current_board_maps_both_module_slots);
    RUN_TEST(test_registered_modules_are_compatible_with_both_current_slots);
    RUN_TEST(test_compatibility_reports_missing_profile_slot_and_resources);
    RUN_TEST(test_store_commits_magic_last_and_verifies_readback);
    RUN_TEST(test_store_reports_write_and_readback_failures);
    RUN_TEST(test_discovery_reads_slots_independently_and_preserves_generic_fallback);
    RUN_TEST(test_discovery_can_rescan_without_retaining_stale_identity);
    return UNITY_END();
}
