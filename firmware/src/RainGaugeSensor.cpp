#include "RainGaugeSensor.h"

namespace WeatherStation {

RainGaugeSensor::RainGaugeSensor(
    SensorId id,
    const RainGaugeConfiguration& configuration,
    ILogger& logger)
    : id_(id)
    , configuration_(configuration)
    , logger_(logger) {
}

RainGaugeSensor::~RainGaugeSensor() {
    if (interruptAttached_) {
        detachInterrupt(digitalPinToInterrupt(configuration_.gpio.number));
        portENTER_CRITICAL(&interruptMux_);
        pendingTipCount_ = 0;
        portEXIT_CRITICAL(&interruptMux_);
        interruptAttached_ = false;
    }
}

SensorId RainGaugeSensor::id() const {
    return id_;
}

const char* RainGaugeSensor::type() const {
    return "Rain Gauge";
}

SensorProvenance RainGaugeSensor::provenance() const {
    return SensorProvenance::Physical;
}

SensorState RainGaugeSensor::state() const {
    return state_;
}

bool RainGaugeSensor::supports(MeasurementType type) const {
    return type == MeasurementType::RainGaugeTip
        || type == MeasurementType::RainfallIncrement;
}

void RainGaugeSensor::begin() {
    state_ = SensorState::Initializing;
    pinMode(configuration_.gpio.number, INPUT_PULLUP);
    pendingTipCount_ = 0;
    const uint32_t debounceMicros = configuration_.debounceMs * 1000UL;
    lastAcceptedTipMicros_ = micros() - debounceMicros;
    attachInterruptArg(
        digitalPinToInterrupt(configuration_.gpio.number),
        &RainGaugeSensor::handleInterrupt,
        this,
        FALLING);
    interruptAttached_ = true;
    state_ = SensorState::Ready;
    logger_.printf("RainGauge sensor %u initialized on GPIO%u\n",
        id_, configuration_.gpio.number);
}

void IRAM_ATTR RainGaugeSensor::handleInterrupt(void* argument) {
    static_cast<RainGaugeSensor*>(argument)->recordTip();
}

void IRAM_ATTR RainGaugeSensor::recordTip() {
    const uint32_t nowMicros = micros();
    const uint32_t debounceMicros = configuration_.debounceMs * 1000UL;
    portENTER_CRITICAL_ISR(&interruptMux_);
    if (static_cast<uint32_t>(nowMicros - lastAcceptedTipMicros_) >= debounceMicros) {
        lastAcceptedTipMicros_ = nowMicros;
        if (pendingTipCount_ != UINT32_MAX) ++pendingTipCount_;
    }
    portEXIT_CRITICAL_ISR(&interruptMux_);
}

SensorOperationResult RainGaugeSensor::service(IMeasurementSink& output) {
    if (state_ != SensorState::Ready && state_ != SensorState::Degraded) {
        return SensorOperationResult::NoData;
    }
    uint32_t pendingTips = 0;
    portENTER_CRITICAL(&interruptMux_);
    pendingTips = pendingTipCount_;
    pendingTipCount_ = 0;
    portEXIT_CRITICAL(&interruptMux_);
    if (pendingTips == 0) return SensorOperationResult::NoData;
    for (uint32_t index = 0; index < pendingTips; ++index) emitTip(output);
    return SensorOperationResult::Completed;
}

SensorOperationResult RainGaugeSensor::sample(IMeasurementSink& output) {
    (void)output;
    return SensorOperationResult::NoData;
}

void RainGaugeSensor::emitTip(IMeasurementSink& output) {
    Measurement tip;
    tip.type = MeasurementType::RainGaugeTip;
    tip.value = MeasurementValue::none();
    tip.valid = true;
    tip.quality = MeasurementQuality::Good;
    output.emit(tip);

    Measurement increment;
    increment.type = MeasurementType::RainfallIncrement;
    increment.value = MeasurementValue::floatingPoint(configuration_.millimetersPerTip);
    increment.valid = true;
    increment.quality = MeasurementQuality::Good;
    output.emit(increment);
}

} // namespace WeatherStation
