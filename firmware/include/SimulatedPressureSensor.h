#pragma once

#include <cstdint>

#include "IMonotonicClock.h"
#include "ISensor.h"

namespace WeatherStation {

class SimulatedPressureSensor : public ISensor {
public:
    SimulatedPressureSensor(SensorId id, IMonotonicClock& monotonicClock);

    SensorId id() const override;
    SensorProvenance provenance() const override;
    SensorState state() const override;
    bool supports(MeasurementType type) const override;

    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;

private:
    SensorId id_;
    IMonotonicClock& monotonicClock_;
    SensorState state_ = SensorState::Unknown;
    uint32_t lastMonotonicMs_ = 0;
    uint64_t elapsedMs_ = 0;
};

} // namespace WeatherStation
