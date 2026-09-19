#include "HardwareDescriptorCodec.h"

#include <cstring>

namespace EnvNode {
namespace {

using Key = HardwareDescriptorKey;

uint64_t bit(Key key) {
    return UINT64_C(1) << static_cast<uint8_t>(key);
}

uint8_t capabilityCode(const DescriptorIdentifier& identifier) {
    if (identifier.coded) {
        return identifier.code > 0 && identifier.code < 32
            ? static_cast<uint8_t>(identifier.code)
            : 0;
    }
    if (identifier.text.equals("digital-input")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::DigitalInput);
    if (identifier.text.equals("digital-output")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::DigitalOutput);
    if (identifier.text.equals("analog-input")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::AnalogInput);
    if (identifier.text.equals("supply")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::Supply);
    if (identifier.text.equals("actuator.on-off")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::ActuatorOnOff);
    if (identifier.text.equals("sensor.pressure")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::SensorPressure);
    if (identifier.text.equals("sensor.current")) return static_cast<uint8_t>(HardwareDescriptorCapabilityCode::SensorCurrent);
    return 0;
}

class Parser {
public:
    Parser(const uint8_t* payload, size_t size)
        : reader_(payload, size) {
    }

    HardwareDescriptorDecodeStatus parse(
        HardwareDescriptorObjectKind expectedKind,
        HardwareDescriptor& descriptor) {
        descriptor = {};
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) return invalidCbor();
        uint64_t seen = 0;
        uint64_t lastKey = 0;
        bool hasLastKey = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, lastKey, hasLastKey)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            switch (static_cast<Key>(key)) {
                case Key::SchemaVersion: parseSchemaVersion(); break;
                case Key::ObjectKind: parseObjectKind(descriptor); break;
                case Key::Identity: parseIdentity(descriptor); break;
                case Key::Compatibility: parseCompatibility(descriptor.compatibility); break;
                case Key::Description: parseDescription(descriptor); break;
                case Key::Hardware: parseHardware(descriptor); break;
                case Key::Manufacturing: parseManufacturing(descriptor); break;
                case Key::Calibration: parseCalibration(descriptor); break;
                default: skip(); break;
            }
        }
        const uint64_t required = bit(Key::SchemaVersion) | bit(Key::ObjectKind)
            | bit(Key::Identity) | bit(Key::Compatibility) | bit(Key::Description)
            | bit(Key::Hardware) | bit(Key::Manufacturing) | bit(Key::Calibration);
        if (ok() && (seen & required) != required) {
            status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
        }
        if (ok() && descriptor.objectKind != expectedKind) {
            status_ = HardwareDescriptorDecodeStatus::ObjectKindMismatch;
        }
        if (ok() && reader_.remaining() != 0) {
            status_ = HardwareDescriptorDecodeStatus::TrailingData;
        }
        return status_;
    }

private:
    bool ok() const { return status_ == HardwareDescriptorDecodeStatus::Valid; }

    HardwareDescriptorDecodeStatus invalidCbor() {
        if (ok()) status_ = HardwareDescriptorDecodeStatus::InvalidCbor;
        return status_;
    }

    bool readKey(uint64_t& key, uint64_t& lastKey, bool& hasLastKey) {
        if (!reader_.readUnsigned(key)) {
            invalidCbor();
            return false;
        }
        if (hasLastKey && key <= lastKey) {
            status_ = HardwareDescriptorDecodeStatus::DuplicateOrUnorderedKey;
            return false;
        }
        hasLastKey = true;
        lastKey = key;
        return true;
    }

    bool readUnsigned(uint64_t& value, uint64_t maximum) {
        if (!reader_.readUnsigned(value)) {
            invalidCbor();
            return false;
        }
        if (value > maximum) {
            status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            return false;
        }
        return true;
    }

    bool readText(DescriptorTextView& text, bool allowEmpty = false) {
        const char* data = nullptr;
        size_t size = 0;
        if (!reader_.readText(data, size)) {
            invalidCbor();
            return false;
        }
        if (!allowEmpty && size == 0) {
            status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            return false;
        }
        text.data = data;
        text.size = size;
        return true;
    }

    bool readNullableText(DescriptorTextView& text) {
        uint8_t major = 0;
        if (!reader_.peekMajorType(major)) {
            invalidCbor();
            return false;
        }
        if (major == 7) {
            if (!reader_.readNull()) invalidCbor();
            return ok();
        }
        return readText(text, true);
    }

    bool readIdentifier(DescriptorIdentifier& identifier) {
        uint8_t major = 0;
        if (!reader_.peekMajorType(major)) {
            invalidCbor();
            return false;
        }
        if (major == 0) {
            uint64_t code = 0;
            if (!readUnsigned(code, UINT16_MAX)) return false;
            identifier.coded = true;
            identifier.code = static_cast<uint16_t>(code);
            return true;
        }
        identifier.coded = false;
        return readText(identifier.text);
    }

    void skip() {
        if (!reader_.skip()) invalidCbor();
    }

    void parseSchemaVersion() {
        size_t count = 0;
        uint64_t major = 0;
        uint64_t minor = 0;
        if (!reader_.enterArray(count) || count != 2
            || !readUnsigned(major, UINT16_MAX)
            || !readUnsigned(minor, UINT16_MAX)) {
            if (ok()) status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            return;
        }
        if (major != 0 || minor != 1) {
            status_ = HardwareDescriptorDecodeStatus::InvalidValue;
        }
    }

    void parseObjectKind(HardwareDescriptor& descriptor) {
        uint64_t value = 0;
        if (!readUnsigned(value, 1)) return;
        descriptor.objectKind = static_cast<HardwareDescriptorObjectKind>(value);
    }

    void parseRevision(DescriptorHardwareRevision& revision) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0, value = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Major)) {
                if (readUnsigned(value, UINT8_MAX)) revision.major = static_cast<uint8_t>(value);
            } else if (key == static_cast<uint8_t>(Key::Minor)) {
                if (readUnsigned(value, UINT8_MAX)) revision.minor = static_cast<uint8_t>(value);
            } else skip();
        }
        const uint64_t required = bit(Key::Major) | bit(Key::Minor);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseIdentity(HardwareDescriptor& descriptor) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::TypeId)) readText(descriptor.typeId);
            else if (key == static_cast<uint8_t>(Key::InstanceId)) {
                const uint8_t* bytes = nullptr;
                size_t size = 0;
                if (!reader_.readByteString(bytes, size)) invalidCbor();
                else if (size != sizeof(descriptor.instanceId)) status_ = HardwareDescriptorDecodeStatus::InvalidValue;
                else std::memcpy(descriptor.instanceId, bytes, size);
            } else if (key == static_cast<uint8_t>(Key::Manufacturer)) readText(descriptor.manufacturer);
            else if (key == static_cast<uint8_t>(Key::HardwareRevision)) parseRevision(descriptor.hardwareRevision);
            else if (key == static_cast<uint8_t>(Key::LegacyProfileId)) {
                uint64_t value = 0;
                if (readUnsigned(value, UINT16_MAX)) {
                    descriptor.hasLegacyProfileId = true;
                    descriptor.legacyProfileId = static_cast<uint16_t>(value);
                }
            } else skip();
        }
        const uint64_t required = bit(Key::TypeId) | bit(Key::InstanceId)
            | bit(Key::Manufacturer) | bit(Key::HardwareRevision);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseSemanticVersion(DescriptorSemanticVersion& version) {
        size_t count = 0;
        uint64_t values[3] = {};
        if (!reader_.enterArray(count) || count != 3) {
            if (reader_.status() != CompactCborStatus::Success) invalidCbor();
            else status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            return;
        }
        for (size_t index = 0; index < 3 && ok(); ++index) readUnsigned(values[index], UINT16_MAX);
        version.major = static_cast<uint16_t>(values[0]);
        version.minor = static_cast<uint16_t>(values[1]);
        version.patch = static_cast<uint16_t>(values[2]);
    }

    void parseContract(DescriptorContract& contract) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Id)) readIdentifier(contract.id);
            else if (key == static_cast<uint8_t>(Key::ApiVersion)) {
                uint64_t value = 0;
                if (readUnsigned(value, UINT16_MAX) && value > 0) contract.apiVersion = static_cast<uint16_t>(value);
                else if (ok()) status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            } else skip();
        }
        const uint64_t required = bit(Key::Id) | bit(Key::ApiVersion);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    template <size_t Capacity>
    void parseIdentifierArray(DescriptorIdentifier (&items)[Capacity], size_t& count) {
        size_t itemCount = 0;
        if (!reader_.enterArray(itemCount)) { invalidCbor(); return; }
        if (itemCount > Capacity) { status_ = HardwareDescriptorDecodeStatus::TooManyItems; return; }
        count = itemCount;
        for (size_t index = 0; index < itemCount && ok(); ++index) readIdentifier(items[index]);
    }

    void parseCapabilitySet(DescriptorCapabilitySet& capabilities) {
        size_t count = 0;
        if (!reader_.enterArray(count)) { invalidCbor(); return; }
        if (count > MaximumDescriptorCapabilities) {
            status_ = HardwareDescriptorDecodeStatus::TooManyItems;
            return;
        }
        for (size_t index = 0; index < count && ok(); ++index) {
            DescriptorIdentifier identifier;
            if (!readIdentifier(identifier)) return;
            const uint8_t code = capabilityCode(identifier);
            if (code != 0) {
                capabilities.known |= UINT32_C(1) << code;
            } else {
                capabilities.hasUnknown = true;
            }
        }
    }

    void parseCompatibility(DescriptorCompatibility& compatibility) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::MinimumFirmwareVersion)) parseSemanticVersion(compatibility.minimumFirmwareVersion);
            else if (key == static_cast<uint8_t>(Key::Platform)) parseContract(compatibility.platform);
            else if (key == static_cast<uint8_t>(Key::SafetyProfile)) parseContract(compatibility.safetyProfile);
            else if (key == static_cast<uint8_t>(Key::Drivers)) {
                size_t count = 0;
                if (!reader_.enterArray(count)) invalidCbor();
                else if (count > MaximumDescriptorDrivers) status_ = HardwareDescriptorDecodeStatus::TooManyItems;
                else {
                    compatibility.driverCount = count;
                    for (size_t item = 0; item < count && ok(); ++item) parseContract(compatibility.drivers[item]);
                }
            } else if (key == static_cast<uint8_t>(Key::Capabilities)) {
                parseIdentifierArray(compatibility.capabilities, compatibility.capabilityCount);
            } else skip();
        }
        const uint64_t required = bit(Key::MinimumFirmwareVersion) | bit(Key::Platform)
            | bit(Key::SafetyProfile) | bit(Key::Drivers) | bit(Key::Capabilities);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseDescription(HardwareDescriptor& descriptor) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Name)) readText(descriptor.name);
            else if (key == static_cast<uint8_t>(Key::Summary)) readText(descriptor.summary);
            else if (key == static_cast<uint8_t>(Key::DocumentationUrl)) readText(descriptor.documentationUrl);
            else skip();
        }
        const uint64_t required = bit(Key::Name) | bit(Key::Summary) | bit(Key::DocumentationUrl);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseBindings(DescriptorBinding* bindings, size_t capacity, size_t& count) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        if (pairs > capacity) { status_ = HardwareDescriptorDecodeStatus::TooManyItems; return; }
        count = pairs;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            readText(bindings[index].name);
            readText(bindings[index].target);
        }
    }

    void parseResource(DescriptorResource& resource) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Id)) readText(resource.id);
            else if (key == static_cast<uint8_t>(Key::Kind)) {
                uint64_t value = 0;
                if (readUnsigned(value, 3)) resource.kind = static_cast<HardwareDescriptorResourceKind>(value);
            } else if (key == static_cast<uint8_t>(Key::Capabilities)) parseCapabilitySet(resource.capabilities);
            else if (key == static_cast<uint8_t>(Key::PlatformBinding)) readText(resource.platformBinding);
            else if (key == static_cast<uint8_t>(Key::VoltageMillivolts)) {
                uint64_t value = 0;
                if (readUnsigned(value, UINT32_MAX)) { resource.voltageMillivolts = static_cast<uint32_t>(value); resource.hasVoltage = true; }
            } else skip();
        }
        const uint64_t required = bit(Key::Id) | bit(Key::Kind) | bit(Key::Capabilities);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseSlot(DescriptorSlot& slot) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Id)) readText(slot.id);
            else if (key == static_cast<uint8_t>(Key::Interface)) readIdentifier(slot.interfaceId);
            else if (key == static_cast<uint8_t>(Key::IdentityAddress)) {
                uint64_t value = 0;
                if (readUnsigned(value, 119) && value >= 8) slot.identityAddress = static_cast<uint8_t>(value);
                else if (ok()) status_ = HardwareDescriptorDecodeStatus::InvalidValue;
            } else if (key == static_cast<uint8_t>(Key::Bindings)) parseBindings(slot.bindings, MaximumDescriptorBindings, slot.bindingCount);
            else skip();
        }
        const uint64_t required = bit(Key::Id) | bit(Key::Interface) | bit(Key::IdentityAddress) | bit(Key::Bindings);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseRequirement(DescriptorRequirement& requirement) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Id)) readText(requirement.id);
            else if (key == static_cast<uint8_t>(Key::Resource)) readText(requirement.resource);
            else if (key == static_cast<uint8_t>(Key::Kind)) {
                uint64_t value = 0;
                if (readUnsigned(value, 3)) requirement.kind = static_cast<HardwareDescriptorResourceKind>(value);
            } else if (key == static_cast<uint8_t>(Key::Capabilities)) parseCapabilitySet(requirement.capabilities);
            else skip();
        }
        const uint64_t required = bit(Key::Id) | bit(Key::Resource) | bit(Key::Kind) | bit(Key::Capabilities);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseDevice(DescriptorDevice& device) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Id)) readText(device.id);
            else if (key == static_cast<uint8_t>(Key::Kind)) {
                uint64_t value = 0;
                if (readUnsigned(value, 2)) device.kind = static_cast<HardwareDescriptorDeviceKind>(value);
            } else if (key == static_cast<uint8_t>(Key::Driver)) parseContract(device.driver);
            else if (key == static_cast<uint8_t>(Key::Bindings)) parseBindings(device.bindings, MaximumDescriptorBindings, device.bindingCount);
            else if (key == static_cast<uint8_t>(Key::Measurements)) parseIdentifierArray(device.measurements, device.measurementCount);
            else if (key == static_cast<uint8_t>(Key::Capabilities)) parseCapabilitySet(device.capabilities);
            else skip();
        }
        const uint64_t required = bit(Key::Id) | bit(Key::Kind) | bit(Key::Driver) | bit(Key::Bindings);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseHardware(HardwareDescriptor& descriptor) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Resources)) {
                size_t count = 0;
                if (!reader_.enterArray(count)) invalidCbor();
                else if (count == 0 || count > MaximumDescriptorResources) status_ = HardwareDescriptorDecodeStatus::TooManyItems;
                else { descriptor.resourceCount = count; for (size_t item = 0; item < count && ok(); ++item) parseResource(descriptor.resources[item]); }
            } else if (key == static_cast<uint8_t>(Key::Slots)) {
                size_t count = 0;
                if (!reader_.enterArray(count)) invalidCbor();
                else if (count > MaximumDescriptorSlots) status_ = HardwareDescriptorDecodeStatus::TooManyItems;
                else { descriptor.slotCount = count; for (size_t item = 0; item < count && ok(); ++item) parseSlot(descriptor.slots[item]); }
            } else if (key == static_cast<uint8_t>(Key::Interface)) readIdentifier(descriptor.interfaceId);
            else if (key == static_cast<uint8_t>(Key::Requirements)) {
                size_t count = 0;
                if (!reader_.enterArray(count)) invalidCbor();
                else if (count == 0 || count > MaximumDescriptorRequirements) status_ = HardwareDescriptorDecodeStatus::TooManyItems;
                else { descriptor.requirementCount = count; for (size_t item = 0; item < count && ok(); ++item) parseRequirement(descriptor.requirements[item]); }
            } else if (key == static_cast<uint8_t>(Key::Devices)) {
                size_t count = 0;
                if (!reader_.enterArray(count)) invalidCbor();
                else if (count == 0 || count > MaximumDescriptorDevices) status_ = HardwareDescriptorDecodeStatus::TooManyItems;
                else { descriptor.deviceCount = count; for (size_t item = 0; item < count && ok(); ++item) parseDevice(descriptor.devices[item]); }
            } else skip();
        }
        const uint64_t boardRequired = bit(Key::Resources) | bit(Key::Slots);
        const uint64_t moduleRequired = bit(Key::Interface) | bit(Key::Requirements) | bit(Key::Devices);
        const uint64_t required = descriptor.objectKind == HardwareDescriptorObjectKind::Board
            ? boardRequired : moduleRequired;
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseManufacturing(HardwareDescriptor& descriptor) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::SerialNumber)) readNullableText(descriptor.serialNumber);
            else if (key == static_cast<uint8_t>(Key::ProductionBatch)) readNullableText(descriptor.productionBatch);
            else if (key == static_cast<uint8_t>(Key::ProductionDate)) readNullableText(descriptor.productionDate);
            else skip();
        }
        const uint64_t required = bit(Key::SerialNumber) | bit(Key::ProductionBatch) | bit(Key::ProductionDate);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseCalibrationEntry(DescriptorCalibrationEntry& entry) {
        size_t pairs = 0;
        if (!reader_.enterMap(pairs)) { invalidCbor(); return; }
        uint64_t seen = 0, last = 0;
        bool hasLast = false;
        for (size_t index = 0; index < pairs && ok(); ++index) {
            uint64_t key = 0;
            if (!readKey(key, last, hasLast)) break;
            if (key < 64) seen |= UINT64_C(1) << key;
            if (key == static_cast<uint8_t>(Key::Target)) readText(entry.target);
            else if (key == static_cast<uint8_t>(Key::Schema)) parseContract(entry.schema);
            else if (key == static_cast<uint8_t>(Key::Values)) skip();
            else skip();
        }
        const uint64_t required = bit(Key::Target) | bit(Key::Schema) | bit(Key::Values);
        if (ok() && (seen & required) != required) status_ = HardwareDescriptorDecodeStatus::MissingRequiredField;
    }

    void parseCalibration(HardwareDescriptor& descriptor) {
        size_t count = 0;
        if (!reader_.enterArray(count)) { invalidCbor(); return; }
        if (count > MaximumDescriptorCalibrationEntries) { status_ = HardwareDescriptorDecodeStatus::TooManyItems; return; }
        descriptor.calibrationCount = count;
        for (size_t index = 0; index < count && ok(); ++index) parseCalibrationEntry(descriptor.calibration[index]);
    }

    CompactCborReader reader_;
    HardwareDescriptorDecodeStatus status_ = HardwareDescriptorDecodeStatus::Valid;
};

} // namespace

HardwareDescriptorDecodeStatus HardwareDescriptorCodec::decode(
    const uint8_t* payload,
    size_t payloadSize,
    HardwareDescriptorObjectKind expectedKind,
    HardwareDescriptor& descriptor) {
    if (payload == nullptr || payloadSize == 0) {
        return HardwareDescriptorDecodeStatus::InvalidCbor;
    }
    Parser parser(payload, payloadSize);
    return parser.parse(expectedKind, descriptor);
}

const char* hardwareDescriptorDecodeStatusName(HardwareDescriptorDecodeStatus status) {
    switch (status) {
        case HardwareDescriptorDecodeStatus::Valid: return "Valid";
        case HardwareDescriptorDecodeStatus::InvalidCbor: return "InvalidCbor";
        case HardwareDescriptorDecodeStatus::MissingRequiredField: return "MissingRequiredField";
        case HardwareDescriptorDecodeStatus::DuplicateOrUnorderedKey: return "DuplicateOrUnorderedKey";
        case HardwareDescriptorDecodeStatus::InvalidValue: return "InvalidValue";
        case HardwareDescriptorDecodeStatus::TooManyItems: return "TooManyItems";
        case HardwareDescriptorDecodeStatus::ObjectKindMismatch: return "ObjectKindMismatch";
        case HardwareDescriptorDecodeStatus::TrailingData: return "TrailingData";
        default: return "InvalidCbor";
    }
}

} // namespace EnvNode
