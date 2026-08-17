#pragma once

#include "IMeasurementResolver.h"
#include "IMeasurementObserver.h"
#include "SensorSlotConfiguration.h"

namespace EnvNode {

class MeasurementSnapshotCache
    : public IMeasurementObserver
    , public IMeasurementResolver {
public:
    static constexpr size_t MaximumEntryCount =
        MaxSensorSlotCount * SupportedMeasurementTypeCount;

    void observe(const Measurement& measurement, uint32_t acceptedMonotonicMs) override;
    void clear() override;
    bool latest(
        const MeasurementSourceReference& source,
        MeasurementSnapshot& result) const override;
    bool snapshot(
        SensorId sensorId,
        MeasurementType type,
        MeasurementSnapshot& result) const;

private:
    struct Entry {
        bool occupied = false;
        MeasurementSnapshot snapshot;
    };

    static bool entryIndex(SensorId sensorId, MeasurementType type, size_t& index);
    uint32_t advanceRevision();

    Entry entries_[MaximumEntryCount];
    uint32_t revisionCounter_ = 0;
};

} // namespace EnvNode
