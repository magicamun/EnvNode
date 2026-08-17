#include "MqttTopic.h"

#include <cstdlib>
#include <cstring>

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

String mqttActuatorCommandSubscription(const String& deviceName) {
    return mqttDeviceTopicRoot(deviceName) + "/actuator/+/cmd/on_off";
}

String mqttActuatorCommandTopic(const String& deviceName, ActuatorId id) {
    return mqttDeviceTopicRoot(deviceName) + "/actuator/"
        + String(static_cast<unsigned int>(id)) + "/cmd/on_off";
}

String mqttActuatorStatusTopic(const String& deviceName, ActuatorId id) {
    return mqttDeviceTopicRoot(deviceName) + "/actuator/"
        + String(static_cast<unsigned int>(id)) + "/status/on_off";
}

bool parseMqttActuatorCommandTopic(
    const char* topic,
    const String& deviceName,
    ActuatorId& id) {
    if (topic == nullptr) return false;
    const String prefixString = mqttDeviceTopicRoot(deviceName) + "/actuator/";
    const char* prefix = prefixString.c_str();
    const size_t prefixLength = strlen(prefix);
    if (strncmp(topic, prefix, prefixLength) != 0) return false;

    const char* slotStart = topic + prefixLength;
    if (*slotStart < '0' || *slotStart > '9') return false;
    char* slotEnd = nullptr;
    const unsigned long parsed = strtoul(slotStart, &slotEnd, 10);
    if (slotEnd == slotStart
        || strcmp(slotEnd, "/cmd/on_off") != 0
        || parsed == 0
        || parsed > 0xFFFFUL) {
        return false;
    }
    id = static_cast<ActuatorId>(parsed);
    return true;
}

} // namespace EnvNode
