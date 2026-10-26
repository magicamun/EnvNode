#include "DuoRelayDescriptor.h"

#include "HardwareDescriptorVocabulary.h"

namespace EnvNode {
namespace {

using Key = HardwareDescriptorKey;

void key(CompactCborWriter& writer, Key value) {
    writer.writeUnsigned(static_cast<uint8_t>(value));
}

void contract(CompactCborWriter& writer, uint8_t identifier) {
    writer.beginMap(2);
    key(writer, Key::Id); writer.writeUnsigned(identifier);
    key(writer, Key::ApiVersion); writer.writeUnsigned(1);
}

void nullableText(CompactCborWriter& writer, const char* value) {
    if (value == nullptr || value[0] == '\0') writer.writeNull();
    else writer.writeText(value);
}

void requirement(
    CompactCborWriter& writer,
    const char* id,
    const char* resource,
    HardwareDescriptorResourceKind kind,
    HardwareDescriptorCapabilityCode capability,
    bool includeVoltage) {
    writer.beginMap(includeVoltage ? 5 : 4);
    key(writer, Key::Capabilities);
    writer.beginArray(1); writer.writeUnsigned(static_cast<uint8_t>(capability));
    key(writer, Key::Id); writer.writeText(id);
    key(writer, Key::Kind); writer.writeUnsigned(static_cast<uint8_t>(kind));
    if (includeVoltage) {
        key(writer, Key::Properties);
        writer.beginMap(1); writer.writeText("nominalMillivolts"); writer.writeUnsigned(5000);
    }
    key(writer, Key::Resource); writer.writeText(resource);
}

void relayDevice(
    CompactCborWriter& writer,
    const char* id,
    const char* requirementId) {
    writer.beginMap(6);
    key(writer, Key::Capabilities);
    writer.beginArray(1);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff));
    key(writer, Key::Id); writer.writeText(id);
    key(writer, Key::Kind);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorDeviceKind::Actuator));
    key(writer, Key::Bindings);
    writer.beginMap(1); writer.writeText("output"); writer.writeText(requirementId);
    key(writer, Key::Driver);
    contract(writer, static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff));
    key(writer, Key::Parameters);
    writer.beginMap(2);
    writer.writeText("safeLevel"); writer.writeText("low");
    writer.writeText("activeLevel"); writer.writeText("high");
}

} // namespace

CompactCborStatus DuoRelayDescriptor::encode(
    const DuoRelayDescriptorManufacturingData& manufacturing,
    uint8_t* output,
    size_t capacity,
    size_t& encodedSize) {
    encodedSize = 0;
    CompactCborWriter writer(output, capacity);
    writer.beginMap(8);
    key(writer, Key::SchemaVersion);
    writer.beginArray(2); writer.writeUnsigned(0); writer.writeUnsigned(1);
    key(writer, Key::ObjectKind); writer.writeUnsigned(1);
    key(writer, Key::Identity);
    writer.beginMap(4);
    key(writer, Key::TypeId); writer.writeText("org.envnode.module.duo-relay");
    key(writer, Key::InstanceId);
    writer.writeByteString(manufacturing.instanceId, sizeof(manufacturing.instanceId));
    key(writer, Key::Manufacturer); writer.writeText("org.envnode");
    key(writer, Key::HardwareRevision);
    writer.beginMap(2);
    key(writer, Key::Major); writer.writeUnsigned(0);
    key(writer, Key::Minor); writer.writeUnsigned(3);

    key(writer, Key::Compatibility);
    writer.beginMap(5);
    key(writer, Key::MinimumFirmwareVersion);
    writer.beginArray(3); writer.writeUnsigned(0); writer.writeUnsigned(5); writer.writeUnsigned(0);
    key(writer, Key::Platform);
    contract(writer, static_cast<uint8_t>(HardwareDescriptorPlatformCode::Any));
    key(writer, Key::SafetyProfile);
    contract(writer, static_cast<uint8_t>(HardwareDescriptorSafetyProfileCode::ModuleInterface));
    key(writer, Key::Drivers);
    writer.beginArray(1);
    contract(writer, static_cast<uint8_t>(HardwareDescriptorDriverCode::GpioOnOff));
    key(writer, Key::Capabilities);
    writer.beginArray(1);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff));

    key(writer, Key::Description);
    writer.beginMap(3);
    key(writer, Key::Name); writer.writeText("EnvNode DuoRelay");
    key(writer, Key::Summary);
    writer.writeText("Two independently controlled 5 V electromechanical changeover relays.");
    key(writer, Key::DocumentationUrl);
    writer.writeText("https://github.com/magicamun/EnvNode/tree/main/hardware/kicad/Modules/FullSize/DuoRelay");

    key(writer, Key::Hardware);
    writer.beginMap(3);
    key(writer, Key::Interface);
    writer.writeUnsigned(static_cast<uint8_t>(HardwareDescriptorInterfaceCode::Module2x7));
    key(writer, Key::Requirements);
    writer.beginArray(3);
    requirement(writer, "relay1-control", "AUX_GPIO1",
        HardwareDescriptorResourceKind::Gpio,
        HardwareDescriptorCapabilityCode::DigitalOutput, false);
    requirement(writer, "relay2-control", "AUX_GPIO2",
        HardwareDescriptorResourceKind::Gpio,
        HardwareDescriptorCapabilityCode::DigitalOutput, false);
    requirement(writer, "relay-supply", "+5V",
        HardwareDescriptorResourceKind::Power,
        HardwareDescriptorCapabilityCode::Supply, true);
    key(writer, Key::Devices);
    writer.beginArray(2);
    relayDevice(writer, "relay.1", "relay1-control");
    relayDevice(writer, "relay.2", "relay2-control");

    key(writer, Key::Manufacturing);
    writer.beginMap(3);
    key(writer, Key::SerialNumber); nullableText(writer, manufacturing.serialNumber);
    key(writer, Key::ProductionBatch); nullableText(writer, manufacturing.productionBatch);
    key(writer, Key::ProductionDate); nullableText(writer, manufacturing.productionDate);
    key(writer, Key::Calibration); writer.beginArray(0);

    if (writer.status() == CompactCborStatus::Success) encodedSize = writer.size();
    return writer.status();
}

} // namespace EnvNode
