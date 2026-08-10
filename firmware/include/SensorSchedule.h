#pragma once

#include <cstdint>

namespace EnvNode {

enum class AcquisitionMode {
    EventOnly,
    Periodic,
};

struct SensorSchedule {
    bool enabled = false;
    AcquisitionMode acquisitionMode = AcquisitionMode::EventOnly;
    uint32_t sampleIntervalMs = 0;

    static SensorSchedule eventOnly(bool enabled = true) {
        SensorSchedule schedule;
        schedule.enabled = enabled;
        schedule.acquisitionMode = AcquisitionMode::EventOnly;
        return schedule;
    }

    static SensorSchedule periodic(uint32_t intervalMs, bool enabled = true) {
        SensorSchedule schedule;
        schedule.enabled = enabled;
        schedule.acquisitionMode = AcquisitionMode::Periodic;
        schedule.sampleIntervalMs = intervalMs;
        return schedule;
    }
};

} // namespace EnvNode
