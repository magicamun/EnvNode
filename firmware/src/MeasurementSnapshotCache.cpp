#include "MeasurementSnapshotCache.h"

namespace EnvNode {

bool MeasurementSnapshotCache::entryIndex(
    SensorId sensorId,
    MeasurementType type,
    size_t& index) {
    if (!isValidSensorId(sensorId)
        || sensorId > MaxSensorSlotCount
        || !isSupportedMeasurementType(type)) {
        return false;
    }
    index = (static_cast<size_t>(sensorId) - 1) * SupportedMeasurementTypeCount
        + static_cast<uint8_t>(type) - 1;
    return index < MaximumEntryCount;
}

void MeasurementSnapshotCache::observe(
    const Measurement& measurement,
    uint32_t acceptedMonotonicMs) {
    size_t index = 0;
    if (!entryIndex(measurement.source, measurement.type, index)) return;
    entries_[index].occupied = true;
    entries_[index].snapshot.measurement = measurement;
    entries_[index].snapshot.acceptedMonotonicMs = acceptedMonotonicMs;
    entries_[index].snapshot.revision = advanceRevision();
}

void MeasurementSnapshotCache::clear() {
    advanceRevision();
    for (size_t index = 0; index < MaximumEntryCount; ++index) {
        entries_[index] = Entry{};
    }
}

bool MeasurementSnapshotCache::latest(
    const MeasurementSourceReference& source,
    MeasurementSnapshot& result) const {
    size_t index = 0;
    if (!entryIndex(source.sensorId, source.measurementType, index)
        || !entries_[index].occupied) {
        return false;
    }
    result = entries_[index].snapshot;
    return true;
}

bool MeasurementSnapshotCache::snapshot(
    SensorId sensorId,
    MeasurementType type,
    MeasurementSnapshot& result) const {
    return latest(MeasurementSourceReference(sensorId, type), result);
}

uint32_t MeasurementSnapshotCache::advanceRevision() {
    // Revisions are change tokens, not ordered counters. Unsigned wrap therefore
    // remains safe for the only supported operation: equality comparison.
    return ++revisionCounter_;
}

} // namespace EnvNode
