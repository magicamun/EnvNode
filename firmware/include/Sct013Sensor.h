// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
#pragma once
#include "ISensor.h"
#include "IMonotonicClock.h"
#include "Sct013Settings.h"
namespace EnvNode {
struct SctTiming {
    uint32_t shortIntervals=0, longIntervals=0;
    int64_t maxGapUs=0, spanUs=0;
    void interval(int64_t us) {
        if (us<250) ++shortIntervals;
        if (us>1000) ++longIntervals;
        if (us>maxGapUs) maxGapUs=us;
    }
};
struct SctWindow {
#ifdef ENVNODE_SCT_DMA
    uint32_t readErrors=0, channelErrors=0, pollErrors=0, lengthErrors=0;
    uint32_t recoveryAttempts=0, recoverySuccesses=0;
    int lastRecoveryError=0;
    int lastReadError=0, unexpectedChannel=-1;
#endif
    SctTiming timing;
    uint32_t revision=0, endMs=0, timingErrors=0;
    float bias=0, rawMv=0, filteredMv=0, minimum=0, maximum=0;
    bool valid() const { return revision && !timingErrors && bias>=1400 && bias<=1900 && minimum>=150 && maximum<=2450; }
};
class ISctInput {
public:
    virtual ~ISctInput() = default;
    virtual bool begin(uint8_t gpio,uint32_t cutoff)=0;
    virtual bool latest(SctWindow& window)=0;
};
class EspSctInput final : public ISctInput {
public:
    ~EspSctInput();
    bool begin(uint8_t gpio,uint32_t cutoff) override;
    bool latest(SctWindow& window) override;
private:
    int channel_=-1;
};
class Sct013Sensor final : public ISensor {
public:
    Sct013Sensor(SensorId id,uint8_t gpio,const Sct013Settings& settings,ISctInput& input,IMonotonicClock& clock)
      : id_(id),gpio_(gpio),settings_(settings),input_(input),clock_(clock) {}
    SensorId id() const override { return id_; }
    const char* type() const override { return "SCT013"; }
    SensorProvenance provenance() const override { return SensorProvenance::Physical; }
    SensorState state() const override { return state_; }
    bool supports(MeasurementType t) const override { return t==MeasurementType::ElectricCurrent; }
    void begin() override;
    SensorOperationResult service(IMeasurementSink&) override;
    SensorOperationResult sample(IMeasurementSink&) override;
private:
    SensorId id_; uint8_t gpio_; Sct013Settings settings_; ISctInput& input_; IMonotonicClock& clock_;
    SctWindow window_; SensorState state_=SensorState::Unknown;
    bool active_=false; float outputA_=0;
};
}
