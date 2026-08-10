#include "MqttTopic.h"

namespace EnvNode {
namespace {

constexpr char MqttTopicRoot[] = "envnode";

bool isTopicSafeCharacter(char character) {
    return (character >= 'a' && character <= 'z')
        || (character >= 'A' && character <= 'Z')
        || (character >= '0' && character <= '9')
        || character == '_'
        || character == '-';
}

} // namespace

const char* mqttTopicRoot() {
    return MqttTopicRoot;
}

String mqttTopicSafeDeviceName(const String& deviceName) {
    if (deviceName.isEmpty()) return "device";
    String normalized;
    normalized.reserve(deviceName.length());
    for (size_t index = 0; index < deviceName.length(); ++index) {
        const char character = deviceName.charAt(index);
        normalized += isTopicSafeCharacter(character) ? character : '_';
    }
    return normalized;
}

String mqttDeviceTopicRoot(const String& deviceName) {
    return String(mqttTopicRoot()) + "/" + mqttTopicSafeDeviceName(deviceName);
}

const char* mqttMeasurementTypeTopic(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature: return "temperature";
        case MeasurementType::RelativeHumidity: return "relative_humidity";
        case MeasurementType::AtmosphericPressure: return "atmospheric_pressure";
        case MeasurementType::SolarIrradiance: return "solar_irradiance";
        case MeasurementType::SolarCellTemperature: return "solar_cell_temperature";
        case MeasurementType::RainDetectorLevel: return "rain_detector_level";
        case MeasurementType::RainDetectorWet: return "rain_detector_wet";
        case MeasurementType::RainGaugeTip: return "rain_gauge_tip";
        case MeasurementType::RainfallIncrement: return "rainfall_increment";
        case MeasurementType::Unknown:
        default: return nullptr;
    }
}

} // namespace EnvNode
