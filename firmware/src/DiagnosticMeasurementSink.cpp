#include "DiagnosticMeasurementSink.h"

namespace WeatherStation {

DiagnosticMeasurementSink::DiagnosticMeasurementSink(ILogger& logger)
    : logger_(logger) {
}

void DiagnosticMeasurementSink::emit(const Measurement& measurement) {
    float value = 0.0F;
    const bool hasFloatingPointValue = measurement.value.tryGetFloatingPoint(value);

    if (!hasFloatingPointValue) {
        logger_.printf(
            "Measurement: sensor=%u type=%s value=<none> valid=%s simulated=%s timestamp=%lld\n",
            static_cast<unsigned int>(measurement.source),
            measurementTypeName(measurement.type),
            measurement.valid ? "true" : "false",
            measurement.provenance == SensorProvenance::Simulated ? "true" : "false",
            static_cast<long long>(measurement.timestamp));
        return;
    }

    logger_.printf(
        "Measurement: sensor=%u type=%s value=%.2f valid=%s simulated=%s timestamp=%lld\n",
        static_cast<unsigned int>(measurement.source),
        measurementTypeName(measurement.type),
        static_cast<double>(value),
        measurement.valid ? "true" : "false",
        measurement.provenance == SensorProvenance::Simulated ? "true" : "false",
        static_cast<long long>(measurement.timestamp));
}

const char* DiagnosticMeasurementSink::measurementTypeName(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature:
            return "Temperature";
        case MeasurementType::RelativeHumidity:
            return "RelativeHumidity";
        case MeasurementType::AtmosphericPressure:
            return "AtmosphericPressure";
        case MeasurementType::SolarIrradiance:
            return "SolarIrradiance";
        case MeasurementType::SolarCellTemperature:
            return "SolarCellTemperature";
        case MeasurementType::RainDetectorLevel:
            return "RainDetectorLevel";
        case MeasurementType::RainDetectorWet:
            return "RainDetectorWet";
        case MeasurementType::RainGaugeTip:
            return "RainGaugeTip";
        case MeasurementType::Unknown:
        default:
            return "Unknown";
    }
}

} // namespace WeatherStation
