// EnvNode-owned: see ../../LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).
#pragma once
#include "ISensor.h"
#include "IRainFrequencyResolver.h"
#include "IMonotonicClock.h"
#include "RainFrequencyAcquisition.h"

namespace EnvNode {
// Physical/simulated input is injected; no heater policy or transport code.
class RainDetectorSensor : public ISensor, private IRainFrequencyObserver {
public:
    RainDetectorSensor(SensorId id, uint8_t gpio, IRainFrequencyInput& input,
        IMonotonicClock& clock) : id_(id), gpio_(gpio), clock_(clock), acquisition_(input,*this) {}
    SensorId id() const override { return id_; }
    const char* type() const override { return "RainDetector"; }
    SensorProvenance provenance() const override { return SensorProvenance::Physical; }
    SensorState state() const override { return state_; }
    bool supports(MeasurementType type) const override { return type == MeasurementType::RainDetectorFrequency; }
    void rainFrequencyHeaterChanged() override {
        if (!acquisition_.begin(gpio_)) state_ = SensorState::Failed;
    }
    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;
    bool rainFrequencySnapshot(RainFrequencySnapshot& result) const override;
private:
    void onRainFrequency(const RainFrequencyWindow& window) override;
    SensorId id_;
    uint8_t gpio_;
    IMonotonicClock& clock_;
    RainFrequencyAcquisition acquisition_;
    SensorState state_ = SensorState::Unknown;
    RainFrequencySnapshot latest_;
    bool observed_ = false;
    uint32_t lastRetryMs_ = 0;
};
} // namespace EnvNode
