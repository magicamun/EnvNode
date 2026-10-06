#include "SensorPropertyReader.h"

namespace EnvNode {

SensorPropertyReader::SensorPropertyReader(
    const ISensor& sensor, const IMeasurementResolver& measurements)
    : sensor_(sensor), measurements_(measurements) {}

MeasurementType SensorPropertyReader::resolve(const PropertyReference& reference) const {
    if (reference.componentKind != PropertyComponentKind::Sensor
        || !isValidSensorId(reference.componentId)
        || reference.componentId != sensor_.id()) {
        return MeasurementType::Unknown;
    }
    const MeasurementType type = measurementTypeFromStableId(reference.propertyKey);
    if (measurementTypeMetadata(type).semantics != MeasurementSemantics::State
        || !sensor_.supports(type)) {
        return MeasurementType::Unknown;
    }
    return type;
}

bool SensorPropertyReader::describe(
    const PropertyReference& reference, PropertyDescription& result) const {
    result = PropertyDescription{};
    const MeasurementType type = resolve(reference);
    if (type == MeasurementType::Unknown) return false;

    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(type);
    result.stableKey = metadata.stableId;
    result.displayName = metadata.displayName;
    result.valueKind = propertyValueKind(metadata.expectedValueKind);
    result.canonicalUnit = metadata.canonicalUnit;
    return true;
}

PropertyReadResult SensorPropertyReader::read(
    const PropertyReference& reference, PropertySnapshot& result) const {
    result = PropertySnapshot{};
    const MeasurementType type = resolve(reference);
    if (type == MeasurementType::Unknown) return PropertyReadResult::UnknownReference;

    MeasurementSnapshot snapshot;
    if (!measurements_.latest(MeasurementSourceReference(reference.componentId, type), snapshot)) {
        return PropertyReadResult::NoValue;
    }
    result.hasQuality = true;
    result.hasTimestamp = true;
    result.hasAcceptedMonotonicMs = true;
    result.hasRevision = true;
    result.value = snapshot.measurement.value;
    result.valid = snapshot.measurement.valid;
    result.quality = snapshot.measurement.quality;
    result.timestamp = snapshot.measurement.timestamp;
    result.acceptedMonotonicMs = snapshot.acceptedMonotonicMs;
    result.revision = snapshot.revision;
    return PropertyReadResult::Available;
}

} // namespace EnvNode
