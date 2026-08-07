#pragma once

#include "IMeasurementSink.h"
#include "Logger.h"

namespace WeatherStation {

class DiagnosticMeasurementSink : public IMeasurementSink {
public:
    explicit DiagnosticMeasurementSink(ILogger& logger);

    void emit(const Measurement& measurement) override;

private:
    static const char* measurementTypeName(MeasurementType type);

    ILogger& logger_;
};

} // namespace WeatherStation
