#pragma once

#include <Arduino.h>

#include "ISensor.h"
#include "Logger.h"
#include "SensorSlotConfiguration.h"

namespace WeatherStation {

class RainGaugeSensor : public ISensor {
public:
    RainGaugeSensor(SensorId id, const RainGaugeConfiguration& configuration, ILogger& logger);
    ~RainGaugeSensor() override;

    SensorId id() const override;
    const char* type() const override;
    SensorProvenance provenance() const override;
    SensorState state() const override;
    bool supports(MeasurementType type) const override;

    void begin() override;
    SensorOperationResult service(IMeasurementSink& output) override;
    SensorOperationResult sample(IMeasurementSink& output) override;

private:
    static void IRAM_ATTR handleInterrupt(void* argument);
    void IRAM_ATTR recordTip();
    void emitTip(IMeasurementSink& output);

    SensorId id_;
    RainGaugeConfiguration configuration_;
    ILogger& logger_;
    SensorState state_ = SensorState::Unknown;
    volatile uint32_t pendingTipCount_ = 0;
    volatile uint32_t lastAcceptedTipMicros_ = 0;
    portMUX_TYPE interruptMux_ = portMUX_INITIALIZER_UNLOCKED;
    bool interruptAttached_ = false;
};

} // namespace WeatherStation
