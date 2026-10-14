// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "ISensor.h"
#include "GeoPosition.h"
#include "IMeasurementResolver.h"
#include "IMonotonicClock.h"
namespace EnvNode {
// Borrowed dependencies must outlive this composition. It forwards the local
// measurement unchanged and emits an explicitly derived additional channel.
class SeaLevelPressureSensor : public ISensor, private IMeasurementSink {
public:
    void bind(ISensor& sensor, const GeoPosition& position) {
        sensor_ = &sensor; position_ = &position;
        configuration_ = nullptr; measurements_ = nullptr; clock_ = nullptr;
        status_ = PressureReductionStatus{};
    }
    void bind(ISensor& sensor, const GeoPosition& position,
        const PressureReductionConfiguration& configuration,
        const IMeasurementResolver& measurements, const IMonotonicClock& clock) {
        bind(sensor, position); configuration_ = &configuration; measurements_ = &measurements; clock_ = &clock;
        status_ = PressureReductionStatus{};
    }
    bool pressureReductionStatus(PressureReductionStatus& status) const override {
        if (!supports(MeasurementType::SeaLevelPressure)) return false;
        status = status_; return true;
    }
    SensorId id() const override { return sensor_->id(); }
    const char* type() const override { return sensor_->type(); }
    SensorProvenance provenance() const override { return sensor_->provenance(); }
    SensorState state() const override { return sensor_->state(); }
    bool supports(MeasurementType type) const override;
    void begin() override { sensor_->begin(); }
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;
private:
    void emit(const Measurement& measurement) override;
    ISensor* sensor_ = nullptr;
    const GeoPosition* position_ = nullptr;
    const PressureReductionConfiguration* configuration_ = nullptr;
    const IMeasurementResolver* measurements_ = nullptr;
    const IMonotonicClock* clock_ = nullptr;
    PressureReductionStatus status_;
    IMeasurementSink* output_ = nullptr;
};
}
