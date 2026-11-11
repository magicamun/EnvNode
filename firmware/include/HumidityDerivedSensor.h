// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "ISensor.h"
namespace EnvNode {
// Only pairs values emitted by the wrapped instance during one operation.
class HumidityDerivedSensor : public ISensor, private IMeasurementSink {
public:
    void bind(ISensor& sensor) { sensor_ = &sensor; }
    SensorId id() const override { return sensor_->id(); }
    const char* type() const override { return sensor_->type(); }
    SensorProvenance provenance() const override { return sensor_->provenance(); }
    SensorState state() const override { return sensor_->state(); }
    bool supports(MeasurementType type) const override;
    bool pressureReductionStatus(PressureReductionStatus& status) const override {
        return sensor_->pressureReductionStatus(status);
    }
    // Preserve direct acquisition capabilities through the full sensor composition.
    void rainFrequencyHeaterChanged() override {
        if (sensor_) sensor_->rainFrequencyHeaterChanged();
    }
    bool rainFrequencySnapshot(RainFrequencySnapshot& result) const override {
        return sensor_ && sensor_->rainFrequencySnapshot(result);
    }
    void begin() override { sensor_->begin(); }
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;
private:
    SensorOperationResult run(IMeasurementSink& output, bool sample);
    void emit(const Measurement& measurement) override;
    void emitDerived();
    ISensor* sensor_ = nullptr;
    IMeasurementSink* output_ = nullptr;
    Measurement temperature_, humidity_;
    bool hasTemperature_ = false, hasHumidity_ = false;
};
// Magnus approximation over liquid water, including supercooled water.
bool humidityDerivedValues(float celsius, float relativeHumidity,
    float& dewPoint, bool& hasDewPoint, float& absoluteHumidity);
}
