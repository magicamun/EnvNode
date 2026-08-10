#pragma once

#include "IMeasurementObserver.h"
#include "SensorSlotConfiguration.h"

namespace EnvNode {

struct MeasurementSnapshot {
    Measurement measurement;
    uint32_t acceptedMonotonicMs = 0;
};

class MeasurementSnapshotCache : public IMeasurementObserver {
public:
    static constexpr size_t MaximumEntryCount =
        MaxSensorSlotCount * SupportedMeasurementTypeCount;

    void observe(const Measurement& measurement, uint32_t acceptedMonotonicMs) override;
    void clear() override;
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

    Entry entries_[MaximumEntryCount];
};

} // namespace EnvNode
