// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See the repository root LICENSE.
#pragma once
#include "ISensor.h"
#include "IConfigurationService.h"
#include "ITimeService.h"
namespace EnvNode {
// Geometric solar centre. Azimuth clockwise from true north, elevation above horizon.
bool solarPosition(const GeoPosition& position, time_t utc, float& azimuth, float& elevation);
class SolarPositionSensor : public ISensor {
public:
    SolarPositionSensor(SensorId id, const IConfigurationService& configuration, const ITimeService& time)
        : id_(id), configuration_(configuration), time_(time) {}
    SensorId id() const override { return id_; }
    const char* type() const override { return "SolarPosition"; }
    SensorProvenance provenance() const override { return SensorProvenance::Calculated; }
    SensorState state() const override { return state_; }
    bool supports(MeasurementType type) const override {
        return type == MeasurementType::SolarAzimuth || type == MeasurementType::SolarElevation;
    }
    void begin() override { state_ = SensorState::Ready; }
    SensorOperationResult service(IMeasurementSink&) override { return SensorOperationResult::NoData; }
    SensorOperationResult sample(IMeasurementSink& output) override;
private:
    SensorId id_;
    const IConfigurationService& configuration_;
    const ITimeService& time_;
    SensorState state_ = SensorState::Unknown;
};
}
