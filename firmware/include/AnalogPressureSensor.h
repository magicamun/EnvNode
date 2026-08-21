#pragma once

#include "AverageAnalogSampler.h"
#include "ISensor.h"
#include "LinearTwoPointCalibration.h"

namespace EnvNode {

struct HydrostaticLevelConfiguration {
    float liquidDensityKgPerCubicMetre;
    float gravitationalAccelerationMetresPerSecondSquared;
};

struct AnalogPressureSensorConfiguration {
    float validInputMinVoltage;
    float validInputMaxVoltage;
    LinearTwoPointCalibration pressureCalibration;
    float minimumReliablePressurePascal;
    bool waterLevelEnabled;
    HydrostaticLevelConfiguration waterLevel;
};

class AnalogPressureSensor : public ISensor {
public:
    AnalogPressureSensor(SensorId id, AverageAnalogSampler& sampler,
        const AnalogPressureSensorConfiguration& configuration);

    SensorId id() const override;
    const char* type() const override;
    SensorProvenance provenance() const override;
    SensorState state() const override;
    bool supports(MeasurementType type) const override;
    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;

private:
    bool configurationValid() const;
    void emitInvalid(IMeasurementSink& output) const;
    void emit(IMeasurementSink& output, MeasurementType type, float value,
        MeasurementQuality quality) const;

    SensorId id_;
    AverageAnalogSampler& sampler_;
    AnalogPressureSensorConfiguration configuration_;
    SensorState state_ = SensorState::Unknown;
};

} // namespace EnvNode
