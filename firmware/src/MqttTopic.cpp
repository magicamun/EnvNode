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

String mqttMeasurementTopic(
    const String& deviceName,
    SensorId sensorId,
    MeasurementType type) {
    const char* typeTopic = mqttMeasurementTypeTopic(type);
    if (!isValidSensorId(sensorId) || typeTopic == nullptr) return String();
    return mqttDeviceTopicRoot(deviceName) + "/sensor/"
        + String(static_cast<unsigned int>(sensorId)) + "/" + typeTopic;
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

String mqttActuatorDescriptionTopic(const String& deviceName, ActuatorId id) {
    return mqttDeviceTopicRoot(deviceName) + "/actuator/"
        + String(static_cast<unsigned int>(id)) + "/description";
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

const char* mqttControllerParameterName(ControllerParameter parameter) {
    const ControllerParameterDescriptor* descriptor =
        controllerParameterDescriptor(parameter);
    return descriptor == nullptr ? nullptr : descriptor->stableName;
}

String mqttControllerCommandSubscription(const String& deviceName) {
    return mqttDeviceTopicRoot(deviceName) + "/controller/+/cmd";
}

String mqttControllerParameterCommandSubscription(const String& deviceName) {
    return mqttDeviceTopicRoot(deviceName) + "/controller/+/cmd/parameter/+";
}

String mqttControllerCommandTopic(const String& deviceName, ControllerId id) {
    return mqttDeviceTopicRoot(deviceName) + "/controller/"
        + String(static_cast<unsigned int>(id)) + "/cmd";
}

String mqttControllerStatusTopic(const String& deviceName, ControllerId id) {
    return mqttDeviceTopicRoot(deviceName) + "/controller/"
        + String(static_cast<unsigned int>(id)) + "/status";
}

String mqttControllerDescriptionTopic(const String& deviceName, ControllerId id) {
    return mqttDeviceTopicRoot(deviceName) + "/controller/"
        + String(static_cast<unsigned int>(id)) + "/description";
}

String mqttControllerParameterTopic(
    const String& deviceName,
    ControllerId id,
    ControllerParameter parameter) {
    const char* name = mqttControllerParameterName(parameter);
    return mqttDeviceTopicRoot(deviceName) + "/controller/"
        + String(static_cast<unsigned int>(id)) + "/parameter/"
        + (name == nullptr ? "" : name);
}

String mqttControllerParameterCommandTopic(
    const String& deviceName,
    ControllerId id,
    ControllerParameter parameter) {
    const char* name = mqttControllerParameterName(parameter);
    return mqttDeviceTopicRoot(deviceName) + "/controller/"
        + String(static_cast<unsigned int>(id)) + "/cmd/parameter/"
        + (name == nullptr ? "" : name);
}

namespace {

bool parseControllerTopicSlot(
    const char* topic,
    const String& deviceName,
    const char*& suffix,
    ControllerId& id) {
    if (topic == nullptr) return false;
    const String prefixString = mqttDeviceTopicRoot(deviceName) + "/controller/";
    const size_t prefixLength = prefixString.length();
    if (strncmp(topic, prefixString.c_str(), prefixLength) != 0) return false;
    const char* slotStart = topic + prefixLength;
    if (*slotStart < '0' || *slotStart > '9') return false;
    char* slotEnd = nullptr;
    const unsigned long parsed = strtoul(slotStart, &slotEnd, 10);
    if (slotEnd == slotStart || parsed == 0 || parsed > 0xFFFFUL) return false;
    id = static_cast<ControllerId>(parsed);
    suffix = slotEnd;
    return true;
}

} // namespace

bool parseMqttControllerCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id) {
    const char* suffix = nullptr;
    return parseControllerTopicSlot(topic, deviceName, suffix, id)
        && strcmp(suffix, "/cmd") == 0;
}

bool parseMqttControllerParameterCommandTopic(
    const char* topic,
    const String& deviceName,
    ControllerId& id,
    ControllerParameter& parameter) {
    const char* suffix = nullptr;
    if (!parseControllerTopicSlot(topic, deviceName, suffix, id)) return false;
    const char prefix[] = "/cmd/parameter/";
    if (strncmp(suffix, prefix, sizeof(prefix) - 1) != 0) return false;
    const char* name = suffix + sizeof(prefix) - 1;
    for (uint8_t value = 0;
         value < static_cast<uint8_t>(ControllerParameter::Count);
         ++value) {
        const ControllerParameter candidate =
            static_cast<ControllerParameter>(value);
        if (strcmp(name, mqttControllerParameterName(candidate)) == 0) {
            parameter = candidate;
            return true;
        }
    }
    return false;
}

} // namespace EnvNode
