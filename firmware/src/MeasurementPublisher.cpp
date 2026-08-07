#include "MeasurementPublisher.h"
#include "UnitConverter.h"

namespace WeatherStation {
namespace {

const bool RetainMeasurements = false;

bool isTopicSafeCharacter(char character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9')
        || character == '_'
        || character == '-';
}

} // namespace

MeasurementPublisher::MeasurementPublisher(
    IConfigurationService& configurationService,
    ITimeService& timeService,
    IMqttService& mqttService)
    : configurationService_(configurationService)
    , timeService_(timeService)
    , mqttService_(mqttService) {
}

void MeasurementPublisher::emit(const Measurement& measurement) {
    if (!isMeasurementStructurallyValid(measurement)) {
        return;
    }

    const char* typeTopic = measurementTypeTopic(measurement.type);
    if (typeTopic == nullptr) {
        return;
    }

    const Configuration& configuration = configurationService_.getConfiguration();
    const String deviceName = topicSafeDeviceName(configuration.device.name);
    const String topic = String("weatherstation/") + deviceName + "/measurement/" + typeTopic;
    const String timestamp = timeService_.iso8601Local(measurement.timestamp);
    const MeasurementTypeMetadata& metadata = measurementTypeMetadata(measurement.type);
    PresentationUnit presentationUnit = configuration.presentationUnitFor(measurement.type);
    if (!supportsPresentationUnit(measurement.type, presentationUnit)) {
        presentationUnit = metadata.defaultPresentationUnit;
    }

    float presentationValue = 0.0F;
    bool hasPresentationValue = false;
    if (measurement.valid && measurement.value.kind() == ValueKind::FloatingPoint) {
        float canonicalValue = 0.0F;
        if (measurement.value.tryGetFloatingPoint(canonicalValue)) {
            hasPresentationValue = UnitConverter::convert(
                measurement.type, canonicalValue, presentationUnit, presentationValue);
            if (!hasPresentationValue && presentationUnit != metadata.defaultPresentationUnit) {
                presentationUnit = metadata.defaultPresentationUnit;
                hasPresentationValue = UnitConverter::convert(
                    measurement.type, canonicalValue, presentationUnit, presentationValue);
            }
        }
    }

    const String payload = serializePayload(
        measurement, timestamp, presentationUnit, presentationValue, hasPresentationValue);

    mqttService_.publish(topic.c_str(), payload.c_str(), RetainMeasurements);
}

const char* MeasurementPublisher::measurementTypeTopic(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature:
            return "temperature";
        case MeasurementType::RelativeHumidity:
            return "relative_humidity";
        case MeasurementType::AtmosphericPressure:
            return "atmospheric_pressure";
        case MeasurementType::SolarIrradiance:
            return "solar_irradiance";
        case MeasurementType::SolarCellTemperature:
            return "solar_cell_temperature";
        case MeasurementType::RainDetectorLevel:
            return "rain_detector_level";
        case MeasurementType::RainDetectorWet:
            return "rain_detector_wet";
        case MeasurementType::RainGaugeTip:
            return "rain_gauge_tip";
        case MeasurementType::Unknown:
        default:
            return nullptr;
    }
}

const char* MeasurementPublisher::qualityName(MeasurementQuality quality) {
    switch (quality) {
        case MeasurementQuality::Good:
            return "good";
        case MeasurementQuality::Estimated:
            return "estimated";
        case MeasurementQuality::Degraded:
            return "degraded";
        default:
            return "degraded";
    }
}

String MeasurementPublisher::topicSafeDeviceName(const String& deviceName) {
    if (deviceName.isEmpty()) {
        return "device";
    }

    String normalized;
    normalized.reserve(deviceName.length());

    for (size_t index = 0; index < deviceName.length(); ++index) {
        const char character = deviceName.charAt(index);
        normalized += isTopicSafeCharacter(character) ? character : '_';
    }

    return normalized;
}

String MeasurementPublisher::serializePayload(
    const Measurement& measurement,
    const String& timestamp,
    PresentationUnit presentationUnit,
    float presentationValue,
    bool hasPresentationValue) {
    String payload;
    payload.reserve(192);
    payload = "{\"timestamp\":\"";
    payload += timestamp;
    payload += '"';

    if (measurement.valid) {
        switch (measurement.value.kind()) {
            case ValueKind::FloatingPoint: {
                if (hasPresentationValue) {
                    payload += ",\"value\":";
                    payload += String(presentationValue, 3);
                }
                break;
            }
            case ValueKind::Boolean: {
                bool value = false;
                if (measurement.value.tryGetBoolean(value)) {
                    payload += ",\"value\":";
                    payload += value ? "true" : "false";
                }
                break;
            }
            case ValueKind::UnsignedInteger: {
                uint32_t value = 0;
                if (measurement.value.tryGetUnsignedInteger(value)) {
                    payload += ",\"value\":";
                    payload += String(value);
                }
                break;
            }
            case ValueKind::None:
                payload += ",\"event\":true";
                break;
        }
    }

    const char* unitSymbol = UnitConverter::symbol(presentationUnit);
    if (measurementTypeMetadata(measurement.type).expectedValueKind == ValueKind::FloatingPoint
        && unitSymbol != nullptr) {
        payload += ",\"unit\":\"";
        payload += unitSymbol;
        payload += '"';
    }

    payload += ",\"valid\":";
    payload += measurement.valid ? "true" : "false";
    payload += ",\"quality\":\"";
    payload += qualityName(measurement.quality);
    payload += "\",\"source\":";
    payload += String(static_cast<unsigned int>(measurement.source));
    payload += ",\"simulated\":";
    payload += measurement.provenance == SensorProvenance::Simulated ? "true" : "false";
    payload += '}';
    return payload;
}

} // namespace WeatherStation
