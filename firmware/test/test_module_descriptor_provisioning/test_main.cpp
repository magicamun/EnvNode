#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "DuoRelayDescriptor.h"
#include "HardwareDescriptorCodec.h"
#include "InstanceUuid.h"
#include "ModuleDescriptorProvisioningService.h"

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
        if (failWrites || address + size > sizeof(bytes)) return false;
        std::memcpy(bytes + address, data, size);
        return true;
    }
    uint8_t bytes[4096];
    size_t writeCalls = 0;
    bool failReads = false;
    bool failWrites = false;
};

const uint8_t InstanceId[16] = {
    0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0x4D, 0xEF,
    0x80, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE,
};

size_t buildDuoRelay(uint8_t* output, size_t capacity) {
    DuoRelayDescriptorManufacturingData data;
    std::memcpy(data.instanceId, InstanceId, sizeof(InstanceId));
    data.serialNumber = "DR-0001";
    data.productionBatch = "2026-09";
    data.productionDate = "2026-09-19";
    size_t size = 0;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::Success),
        static_cast<int>(DuoRelayDescriptor::encode(
            data, output, capacity, size)));
    return size;
}

struct Fixture {
    MemoryStorage slotAStorage;
    MemoryStorage slotBStorage;
    HardwareDescriptorStore slotADescriptor{slotAStorage};
    HardwareDescriptorStore slotBDescriptor{slotBStorage};
    ModuleDiscoveryService discovery{slotADescriptor, slotBDescriptor};
    ModuleDescriptorProvisioningService provisioning{
        slotADescriptor, slotBDescriptor, discovery};
};

} // namespace

void test_duo_relay_template_is_deterministic_valid_and_compatible() {
    uint8_t first[1024];
    uint8_t second[1024];
    const size_t firstSize = buildDuoRelay(first, sizeof(first));
    const size_t secondSize = buildDuoRelay(second, sizeof(second));
    TEST_ASSERT_GREATER_THAN(0, firstSize);
    TEST_ASSERT_LESS_OR_EQUAL(HardwareDescriptorStore::MaximumPayloadSize, firstSize);
    TEST_ASSERT_EQUAL_UINT32(firstSize, secondSize);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(first, second, firstSize);

    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::Valid),
        static_cast<int>(HardwareDescriptorCodec::decode(
            first, firstSize, HardwareDescriptorObjectKind::Module, descriptor)));
    TEST_ASSERT_TRUE(descriptor.typeId.equals("org.envnode.module.duo-relay"));
    TEST_ASSERT_EQUAL_UINT8_ARRAY(InstanceId, descriptor.instanceId, sizeof(InstanceId));
    TEST_ASSERT_TRUE(descriptor.serialNumber.equals("DR-0001"));
    TEST_ASSERT_EQUAL_UINT32(3, descriptor.requirementCount);
    TEST_ASSERT_EQUAL_UINT32(2, descriptor.deviceCount);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::Compatible),
        static_cast<int>(evaluateModuleDescriptorCompatibility(
            descriptor, {0, 5, 0}, currentBoardProfile(), ModuleSlot::A).status));
}

void test_instance_uuid_is_rfc_9562_version_4() {
    uint8_t uuid[InstanceUuid::Size] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0xF6, 0x77,
        0x7F, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    };
    InstanceUuid::makeVersion4(uuid);
    const uint8_t expected[InstanceUuid::Size] = {
        0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x46, 0x77,
        0xBF, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF,
    };
    TEST_ASSERT_EQUAL_UINT8_ARRAY(expected, uuid, sizeof(uuid));
    TEST_ASSERT_EQUAL_HEX8(0x40, uuid[6] & 0xF0);
    TEST_ASSERT_EQUAL_HEX8(0x80, uuid[8] & 0xC0);
}

void test_confirmation_is_required_before_descriptor_write() {
    Fixture fixture;
    uint8_t payload[1024];
    const size_t size = buildDuoRelay(payload, sizeof(payload));
    const ModuleDescriptorProvisioningResult result = fixture.provisioning.provision(
        ModuleSlot::A, payload, size, false);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDescriptorProvisioningStatus::ConfirmationRequired),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writeCalls);
}

void test_first_descriptor_write_uses_bank_b_and_is_rediscovered() {
    Fixture fixture;
    uint8_t payload[1024];
    const size_t size = buildDuoRelay(payload, sizeof(payload));
    const ModuleDescriptorProvisioningResult result = fixture.provisioning.provision(
        ModuleSlot::A, payload, size, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDescriptorProvisioningStatus::Success),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorBank::B), static_cast<int>(result.bank));
    TEST_ASSERT_EQUAL_UINT32(1, result.generation);
    const ModuleDiscoveryResult* discovered = fixture.discovery.result(ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(discovered);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDiscoverySource::Descriptor),
        static_cast<int>(discovered->source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::Compatible),
        static_cast<int>(discovered->descriptorCompatibility));
    TEST_ASSERT_TRUE(discovered->descriptorHasInstanceId);
    TEST_ASSERT_EQUAL_UINT8_ARRAY(
        InstanceId, discovered->descriptorInstanceId, sizeof(InstanceId));
    TEST_ASSERT_TRUE(discovered->descriptorSerialNumber.equals("DR-0001"));
    TEST_ASSERT_TRUE(discovered->descriptorProductionBatch.equals("2026-09"));
    TEST_ASSERT_TRUE(discovered->descriptorProductionDate.equals("2026-09-19"));
}

void test_unavailable_slot_is_not_reported_as_invalid_descriptor() {
    Fixture fixture;
    fixture.slotAStorage.failReads = true;
    fixture.discovery.scan();
    const ModuleDiscoveryResult* discovered = fixture.discovery.result(ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(discovered);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDiscoverySource::None),
        static_cast<int>(discovered->source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::StorageUnavailable),
        static_cast<int>(discovered->descriptorStoreStatus));
    TEST_ASSERT_FALSE(discovered->identified());
}

void test_invalid_descriptor_is_rejected_before_write() {
    Fixture fixture;
    const uint8_t invalid[] = {0xA0};
    const ModuleDescriptorProvisioningResult result = fixture.provisioning.provision(
        ModuleSlot::B, invalid, sizeof(invalid), true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDescriptorProvisioningStatus::InvalidDescriptor),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotBStorage.writeCalls);
}

void test_unassigned_instance_uuid_is_rejected_before_write() {
    Fixture fixture;
    DuoRelayDescriptorManufacturingData data;
    uint8_t payload[1024];
    size_t size = 0;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(CompactCborStatus::Success),
        static_cast<int>(DuoRelayDescriptor::encode(
            data, payload, sizeof(payload), size)));
    const ModuleDescriptorProvisioningResult result = fixture.provisioning.provision(
        ModuleSlot::A, payload, size, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDescriptorProvisioningStatus::InvalidDescriptor),
        static_cast<int>(result.status));
    TEST_ASSERT_EQUAL_UINT32(0, fixture.slotAStorage.writeCalls);
}

void test_storage_failure_is_reported() {
    Fixture fixture;
    fixture.slotBStorage.failReads = true;
    uint8_t payload[1024];
    const size_t size = buildDuoRelay(payload, sizeof(payload));
    const ModuleDescriptorProvisioningResult result = fixture.provisioning.provision(
        ModuleSlot::B, payload, size, true);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDescriptorProvisioningStatus::StorageUnavailable),
        static_cast<int>(result.status));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_duo_relay_template_is_deterministic_valid_and_compatible);
    RUN_TEST(test_instance_uuid_is_rfc_9562_version_4);
    RUN_TEST(test_confirmation_is_required_before_descriptor_write);
    RUN_TEST(test_first_descriptor_write_uses_bank_b_and_is_rediscovered);
    RUN_TEST(test_unavailable_slot_is_not_reported_as_invalid_descriptor);
    RUN_TEST(test_invalid_descriptor_is_rejected_before_write);
    RUN_TEST(test_unassigned_instance_uuid_is_rejected_before_write);
    RUN_TEST(test_storage_failure_is_reported);
    return UNITY_END();
}
