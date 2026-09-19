#include <cstring>

#include <Arduino.h>
#include <unity.h>

#include "HardwareDescriptorCodec.h"
#include "HardwareDescriptorCompatibility.h"
#include "ModuleDiscoveryService.h"

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
        if (address + size > sizeof(bytes)) return false;
        std::memcpy(data, bytes + address, size);
        return true;
    }
    bool write(uint16_t address, const uint8_t* data, size_t size) override {
        if (address + size > sizeof(bytes)) return false;
        std::memcpy(bytes + address, data, size);
        return true;
    }
    uint8_t bytes[4096];
};

void key(CompactCborWriter& writer, HardwareDescriptorKey value) {
    writer.writeUnsigned(static_cast<uint8_t>(value));
}

void version(CompactCborWriter& writer, uint16_t major, uint16_t minor, uint16_t patch) {
    writer.beginArray(3);
    writer.writeUnsigned(major);
    writer.writeUnsigned(minor);
    writer.writeUnsigned(patch);
}

void contract(CompactCborWriter& writer, uint16_t id, uint16_t apiVersion = 1) {
    writer.beginMap(2);
    key(writer, HardwareDescriptorKey::Id);
    writer.writeUnsigned(id);
    key(writer, HardwareDescriptorKey::ApiVersion);
    writer.writeUnsigned(apiVersion);
}

size_t buildModule(uint8_t* output, size_t capacity, bool includeManufacturer = true) {
    CompactCborWriter writer(output, capacity);
    writer.beginMap(9);
    key(writer, HardwareDescriptorKey::SchemaVersion);
    writer.beginArray(2); writer.writeUnsigned(0); writer.writeUnsigned(1);
    key(writer, HardwareDescriptorKey::ObjectKind);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorObjectKind::Module));
    key(writer, HardwareDescriptorKey::Identity);
    writer.beginMap(includeManufacturer ? 5 : 4);
    key(writer, HardwareDescriptorKey::TypeId); writer.writeText("com.example.unknown-module");
    key(writer, HardwareDescriptorKey::InstanceId);
    const uint8_t uuid[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    writer.writeByteString(uuid, sizeof(uuid));
    if (includeManufacturer) {
        key(writer, HardwareDescriptorKey::Manufacturer); writer.writeText("com.example");
    }
    key(writer, HardwareDescriptorKey::HardwareRevision);
    writer.beginMap(2);
    key(writer, HardwareDescriptorKey::Major); writer.writeUnsigned(0);
    key(writer, HardwareDescriptorKey::Minor); writer.writeUnsigned(3);
    key(writer, HardwareDescriptorKey::LegacyProfileId); writer.writeUnsigned(77);
    key(writer, HardwareDescriptorKey::Compatibility);
    writer.beginMap(5);
    key(writer, HardwareDescriptorKey::MinimumFirmwareVersion); version(writer, 0, 5, 0);
    key(writer, HardwareDescriptorKey::Platform); contract(writer, static_cast<uint8_t>(HardwareDescriptorPlatformCode::Any));
    key(writer, HardwareDescriptorKey::SafetyProfile); contract(writer, static_cast<uint8_t>(HardwareDescriptorSafetyProfileCode::ModuleInterface));
    key(writer, HardwareDescriptorKey::Drivers);
    writer.beginArray(1); contract(writer, static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff));
    key(writer, HardwareDescriptorKey::Capabilities);
    writer.beginArray(2);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff));
    writer.writeText("com.example.capability.extra");
    key(writer, HardwareDescriptorKey::Description);
    writer.beginMap(3);
    key(writer, HardwareDescriptorKey::Name); writer.writeText("Unknown relay");
    key(writer, HardwareDescriptorKey::Summary); writer.writeText("Third-party relay module");
    key(writer, HardwareDescriptorKey::DocumentationUrl); writer.writeText("https://example.com/module");
    key(writer, HardwareDescriptorKey::Hardware);
    writer.beginMap(3);
    key(writer, HardwareDescriptorKey::Interface);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorInterfaceCode::Module2x7));
    key(writer, HardwareDescriptorKey::Requirements);
    writer.beginArray(1);
    writer.beginMap(4);
    key(writer, HardwareDescriptorKey::Capabilities);
    writer.beginArray(1); writer.writeText("digital-output");
    key(writer, HardwareDescriptorKey::Id); writer.writeText("relay-control");
    key(writer, HardwareDescriptorKey::Kind); writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorResourceKind::Gpio));
    key(writer, HardwareDescriptorKey::Resource); writer.writeText("AUX_GPIO1");
    key(writer, HardwareDescriptorKey::Devices);
    writer.beginArray(1);
    writer.beginMap(6);
    key(writer, HardwareDescriptorKey::Capabilities);
    writer.beginArray(1); writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff));
    key(writer, HardwareDescriptorKey::Id); writer.writeText("relay.1");
    key(writer, HardwareDescriptorKey::Kind); writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorDeviceKind::Actuator));
    key(writer, HardwareDescriptorKey::Bindings);
    writer.beginMap(1); writer.writeText("output"); writer.writeText("relay-control");
    key(writer, HardwareDescriptorKey::Driver); contract(writer, static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff));
    key(writer, HardwareDescriptorKey::Measurements); writer.beginArray(0);
    key(writer, HardwareDescriptorKey::Manufacturing);
    writer.beginMap(3);
    key(writer, HardwareDescriptorKey::SerialNumber); writer.writeNull();
    key(writer, HardwareDescriptorKey::ProductionBatch); writer.writeNull();
    key(writer, HardwareDescriptorKey::ProductionDate); writer.writeNull();
    key(writer, HardwareDescriptorKey::Calibration); writer.beginArray(0);
    writer.writeUnsigned(46);
    writer.beginMap(1); writer.writeUnsigned(0); writer.beginArray(2); writer.writeBoolean(true); writer.writeNull();
    return writer.status() == CompactCborStatus::Success ? writer.size() : 0;
}

size_t buildBoard(uint8_t* output, size_t capacity) {
    CompactCborWriter writer(output, capacity);
    writer.beginMap(8);
    key(writer, HardwareDescriptorKey::SchemaVersion);
    writer.beginArray(2); writer.writeUnsigned(0); writer.writeUnsigned(1);
    key(writer, HardwareDescriptorKey::ObjectKind); writer.writeUnsigned(0);
    key(writer, HardwareDescriptorKey::Identity);
    writer.beginMap(4);
    key(writer, HardwareDescriptorKey::TypeId); writer.writeText("com.example.board");
    key(writer, HardwareDescriptorKey::InstanceId); const uint8_t uuid[16] = {}; writer.writeByteString(uuid, sizeof(uuid));
    key(writer, HardwareDescriptorKey::Manufacturer); writer.writeText("com.example");
    key(writer, HardwareDescriptorKey::HardwareRevision);
    writer.beginMap(2); key(writer, HardwareDescriptorKey::Major); writer.writeUnsigned(1); key(writer, HardwareDescriptorKey::Minor); writer.writeUnsigned(0);
    key(writer, HardwareDescriptorKey::Compatibility);
    writer.beginMap(5);
    key(writer, HardwareDescriptorKey::MinimumFirmwareVersion); version(writer, 0, 5, 0);
    key(writer, HardwareDescriptorKey::Platform); contract(writer, 99);
    key(writer, HardwareDescriptorKey::SafetyProfile); contract(writer, 99);
    key(writer, HardwareDescriptorKey::Drivers); writer.beginArray(0);
    key(writer, HardwareDescriptorKey::Capabilities); writer.beginArray(0);
    key(writer, HardwareDescriptorKey::Description);
    writer.beginMap(3); key(writer, HardwareDescriptorKey::Name); writer.writeText("Board"); key(writer, HardwareDescriptorKey::Summary); writer.writeText("Board summary"); key(writer, HardwareDescriptorKey::DocumentationUrl); writer.writeText("https://example.com/board");
    key(writer, HardwareDescriptorKey::Hardware);
    writer.beginMap(2);
    key(writer, HardwareDescriptorKey::Resources);
    writer.beginArray(1); writer.beginMap(3);
    key(writer, HardwareDescriptorKey::Capabilities); writer.beginArray(1); writer.writeUnsigned(2);
    key(writer, HardwareDescriptorKey::Id); writer.writeText("gpio.1");
    key(writer, HardwareDescriptorKey::Kind); writer.writeUnsigned(0);
    key(writer, HardwareDescriptorKey::Slots); writer.beginArray(0);
    key(writer, HardwareDescriptorKey::Manufacturing);
    writer.beginMap(3); key(writer, HardwareDescriptorKey::SerialNumber); writer.writeNull(); key(writer, HardwareDescriptorKey::ProductionBatch); writer.writeNull(); key(writer, HardwareDescriptorKey::ProductionDate); writer.writeNull();
    key(writer, HardwareDescriptorKey::Calibration); writer.beginArray(0);
    return writer.status() == CompactCborStatus::Success ? writer.size() : 0;
}

} // namespace

void test_unknown_module_type_decodes_without_registry() {
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload));
    TEST_ASSERT_GREATER_THAN(0, size);
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::Valid),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Module, descriptor)));
    TEST_ASSERT_TRUE(descriptor.typeId.equals("com.example.unknown-module"));
    TEST_ASSERT_TRUE(descriptor.manufacturer.equals("com.example"));
    TEST_ASSERT_EQUAL_UINT8(0, descriptor.hardwareRevision.major);
    TEST_ASSERT_EQUAL_UINT8(3, descriptor.hardwareRevision.minor);
    TEST_ASSERT_EQUAL_UINT16(77, descriptor.legacyProfileId);
    TEST_ASSERT_EQUAL_UINT32(1, descriptor.requirementCount);
    TEST_ASSERT_TRUE(descriptor.requirements[0].resource.equals("AUX_GPIO1"));
    TEST_ASSERT_TRUE(descriptor.requirements[0].capabilities.contains(
        HardwareDescriptorCapabilityCode::DigitalOutput));
    TEST_ASSERT_FALSE(descriptor.requirements[0].capabilities.hasUnknown);
    TEST_ASSERT_EQUAL_UINT32(1, descriptor.deviceCount);
    TEST_ASSERT_TRUE(descriptor.devices[0].id.equals("relay.1"));
    TEST_ASSERT_EQUAL_UINT32(1, descriptor.devices[0].bindingCount);
    TEST_ASSERT_TRUE(descriptor.devices[0].bindings[0].target.equals("relay-control"));
    TEST_ASSERT_EQUAL_UINT32(2, descriptor.compatibility.capabilityCount);
    TEST_ASSERT_FALSE(descriptor.compatibility.capabilities[1].coded);
}

void test_board_resources_decode_independently_of_known_product_id() {
    uint8_t payload[768];
    const size_t size = buildBoard(payload, sizeof(payload));
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::Valid),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Board, descriptor)));
    TEST_ASSERT_TRUE(descriptor.typeId.equals("com.example.board"));
    TEST_ASSERT_EQUAL_UINT32(1, descriptor.resourceCount);
    TEST_ASSERT_TRUE(descriptor.resources[0].id.equals("gpio.1"));
}

void test_missing_required_field_is_rejected() {
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload), false);
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::MissingRequiredField),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Module, descriptor)));
}

void test_expected_object_kind_is_enforced() {
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload));
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::ObjectKindMismatch),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Board, descriptor)));
}

void test_unordered_schema_keys_are_rejected() {
    const uint8_t payload[] = {0xA2, 0x01, 0x01, 0x00, 0x82, 0x00, 0x01};
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::DuplicateOrUnorderedKey),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, sizeof(payload), HardwareDescriptorObjectKind::Module, descriptor)));
}

void test_trailing_data_is_rejected() {
    uint8_t payload[1025];
    size_t size = buildModule(payload, sizeof(payload));
    payload[size++] = 0;
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::TrailingData),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Module, descriptor)));
}

void test_unknown_product_is_compatible_when_contracts_and_resources_are_supported() {
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload));
    HardwareDescriptor descriptor;
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::Valid),
        static_cast<int>(HardwareDescriptorCodec::decode(
            payload, size, HardwareDescriptorObjectKind::Module, descriptor)));
    descriptor.compatibility.capabilityCount = 1;
    const HardwareDescriptorCompatibilityResult result =
        evaluateModuleDescriptorCompatibility(
            descriptor, {0, 5, 0}, currentBoardProfile(), ModuleSlot::A);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::Compatible),
        static_cast<int>(result.status));
}

void test_required_unknown_capability_and_newer_firmware_are_incompatible() {
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload));
    HardwareDescriptor descriptor;
    HardwareDescriptorCodec::decode(
        payload, size, HardwareDescriptorObjectKind::Module, descriptor);
    HardwareDescriptorCompatibilityResult result = evaluateModuleDescriptorCompatibility(
        descriptor, {0, 5, 0}, currentBoardProfile(), ModuleSlot::B);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::MissingCapability),
        static_cast<int>(result.status));

    descriptor.compatibility.capabilityCount = 1;
    descriptor.compatibility.minimumFirmwareVersion = {0, 6, 0};
    result = evaluateModuleDescriptorCompatibility(
        descriptor, {0, 5, 0}, currentBoardProfile(), ModuleSlot::B);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::FirmwareTooOld),
        static_cast<int>(result.status));
}

void test_discovery_prefers_descriptor_and_reports_semantic_compatibility() {
    MemoryStorage slotAStorage;
    MemoryStorage slotBStorage;
    ModuleIdentityStore slotALegacy(slotAStorage);
    ModuleIdentityStore slotBLegacy(slotBStorage);
    HardwareDescriptorStore slotADescriptor(slotAStorage);
    HardwareDescriptorStore slotBDescriptor(slotBStorage);
    uint8_t payload[1024];
    const size_t size = buildModule(payload, sizeof(payload));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorStoreStatus::Valid),
        static_cast<int>(slotADescriptor.write(
            HardwareDescriptorObjectKind::Module, payload, size).status));
    ModuleDiscoveryService discovery(
        slotALegacy, slotBLegacy, slotADescriptor, slotBDescriptor);
    discovery.scan();
    const ModuleDiscoveryResult* result = discovery.result(ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDiscoverySource::Descriptor),
        static_cast<int>(result->source));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorDecodeStatus::Valid),
        static_cast<int>(result->descriptorStatus));
    TEST_ASSERT_TRUE(result->descriptorTypeId.equals("com.example.unknown-module"));
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(HardwareDescriptorCompatibilityStatus::MissingCapability),
        static_cast<int>(result->descriptorCompatibility));
}

void test_discovery_falls_back_to_legacy_only_when_descriptor_is_absent() {
    MemoryStorage slotAStorage;
    MemoryStorage slotBStorage;
    ModuleIdentityStore slotALegacy(slotAStorage);
    ModuleIdentityStore slotBLegacy(slotBStorage);
    HardwareDescriptorStore slotADescriptor(slotAStorage);
    HardwareDescriptorStore slotBDescriptor(slotBStorage);
    const ModuleIdentity legacy = {ModuleProfileId::DuoRelay, {0, 3}, 9};
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleIdentityWriteStatus::Success),
        static_cast<int>(slotALegacy.write(legacy)));
    ModuleDiscoveryService discovery(
        slotALegacy, slotBLegacy, slotADescriptor, slotBDescriptor);
    discovery.scan();
    const ModuleDiscoveryResult* result = discovery.result(ModuleSlot::A);
    TEST_ASSERT_NOT_NULL(result);
    TEST_ASSERT_EQUAL_INT(
        static_cast<int>(ModuleDiscoverySource::LegacyEmidV1),
        static_cast<int>(result->source));
    TEST_ASSERT_EQUAL_UINT32(9, result->identity.serialNumber);
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_unknown_module_type_decodes_without_registry);
    RUN_TEST(test_board_resources_decode_independently_of_known_product_id);
    RUN_TEST(test_missing_required_field_is_rejected);
    RUN_TEST(test_expected_object_kind_is_enforced);
    RUN_TEST(test_unordered_schema_keys_are_rejected);
    RUN_TEST(test_trailing_data_is_rejected);
    RUN_TEST(test_unknown_product_is_compatible_when_contracts_and_resources_are_supported);
    RUN_TEST(test_required_unknown_capability_and_newer_firmware_are_incompatible);
    RUN_TEST(test_discovery_prefers_descriptor_and_reports_semantic_compatibility);
    RUN_TEST(test_discovery_falls_back_to_legacy_only_when_descriptor_is_absent);
    return UNITY_END();
}
