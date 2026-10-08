#pragma once

#include <cstdint>
#include <cstddef>
#include <ctime>
#include <vector>

#include "MeasurementQuality.h"
#include "PropertyValue.h"
#include "PresentationUnit.h"

namespace EnvNode {

enum class PropertyComponentKind : uint8_t { Unknown, Sensor, Actuator, Controller, System, Value };

struct PropertyReference {
    PropertyReference(PropertyComponentKind kind, uint16_t id, const char* key)
        : componentKind(kind), componentId(id), propertyKey(key) {}

    PropertyComponentKind componentKind;
    uint16_t componentId;
    // Borrowed, null-terminated key; only needed for the duration of a call.
    const char* propertyKey;
};

// Optional owned metadata for configurable enums; options point into codes/labels.
struct PropertyEnumMetadata {
    std::vector<String> codes;
    std::vector<String> labels;
    std::vector<PropertyEnumOption> options;
};

struct PropertyDescription {
    // Names have static lifetime; enum options are static or owned by enumMetadata.
    const char* stableKey = "";
    const char* displayName = "";
    PropertyValueKind valueKind = PropertyValueKind::None;
    const PropertyEnumOption* enumOptions = nullptr;
    size_t enumOptionCount = 0;
    std::shared_ptr<const PropertyEnumMetadata> enumMetadata;
    PresentationUnit canonicalUnit = PresentationUnit::None;
    const char* trueText = "True";
    const char* falseText = "False";
};

struct PropertySnapshot {
    PropertyValue value;
    bool valid = false;
    // Optional metadata: consumers must check availability before using it.
    bool hasQuality = false;
    bool hasTimestamp = false;
    bool hasAcceptedMonotonicMs = false;
    bool hasRevision = false;
    MeasurementQuality quality = MeasurementQuality::Good;
    std::time_t timestamp = 0;
    uint32_t acceptedMonotonicMs = 0;
    // Equality-only change token, not an ordered counter.
    uint32_t revision = 0;
};

enum class PropertyReadResult : uint8_t {
    UnknownReference,
    NoValue,
    Available,
};

class IPropertyReader {
public:
    virtual ~IPropertyReader() = default;

    // Both methods reset their output on failure. Available does not imply valid.
    virtual bool describe(
        const PropertyReference& reference, PropertyDescription& result) const = 0;
    virtual PropertyReadResult read(
        const PropertyReference& reference, PropertySnapshot& result) const = 0;
};

} // namespace EnvNode
