#include "HomeAssistantDiscoveryPublisher.h"

#include <Arduino.h>

#include "FirmwareVersion.h"
#include "MqttTopic.h"
#include "UnitConverter.h"

namespace WeatherStation {
namespace {

const MeasurementType DiscoverableMeasurementTypes[] = {
    MeasurementType::Temperature,
    MeasurementType::RelativeHumidity,
    MeasurementType::AtmosphericPressure,
    MeasurementType::SolarIrradiance,
    MeasurementType::SolarCellTemperature,
    MeasurementType::RainDetectorLevel,
    MeasurementType::RainDetectorWet,
    MeasurementType::RainGaugeTip,
    MeasurementType::RainfallIncrement,
};

void appendJsonString(String& output, const char* value) {
    output += '"';
    if (value != nullptr) {
        for (const char* cursor = value; *cursor != '\0'; ++cursor) {
            switch (*cursor) {
                case '"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default:
                    if (static_cast<uint8_t>(*cursor) >= 0x20) output += *cursor;
                    break;
            }
        }
    }
    output += '"';
}

void hashByte(uint32_t& hash, uint8_t value) {
    hash ^= value;
    hash *= 16777619UL;
}

void hashText(uint32_t& hash, const char* value) {
    if (value == nullptr) return;
    while (*value != '\0') hashByte(hash, static_cast<uint8_t>(*value++));
}

bool supports(const SensorRuntimeInfo& info, MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature: return info.supportsTemperature;
        case MeasurementType::RelativeHumidity: return info.supportsRelativeHumidity;
        case MeasurementType::AtmosphericPressure: return info.supportsAtmosphericPressure;
        case MeasurementType::SolarIrradiance: return info.supportsSolarIrradiance;
        case MeasurementType::SolarCellTemperature: return info.supportsSolarCellTemperature;
        case MeasurementType::RainDetectorLevel: return info.supportsRainDetectorLevel;
        case MeasurementType::RainDetectorWet: return info.supportsRainDetectorWet;
        case MeasurementType::RainGaugeTip: return info.supportsRainGaugeTip;
        case MeasurementType::RainfallIncrement: return info.supportsRainfallIncrement;
        default: return false;
    }
}

const char* entityPlatform(MeasurementType type) {
    if (type == MeasurementType::RainGaugeTip) return "event";
    if (type == MeasurementType::RainDetectorWet) return "binary_sensor";
    return "sensor";
}

const char* deviceClass(MeasurementType type) {
    switch (type) {
        case MeasurementType::Temperature:
        case MeasurementType::SolarCellTemperature: return "temperature";
        case MeasurementType::RelativeHumidity: return "humidity";
        case MeasurementType::AtmosphericPressure: return "atmospheric_pressure";
        case MeasurementType::SolarIrradiance: return "irradiance";
        case MeasurementType::RainDetectorWet: return "moisture";
        default: return nullptr;
    }
}

bool hasMeasurementStateClass(MeasurementType type) {
    return measurementTypeMetadata(type).semantics == MeasurementSemantics::State
        && measurementTypeMetadata(type).expectedValueKind == ValueKind::FloatingPoint;
}

} // namespace

HomeAssistantDiscoveryPublisher::HomeAssistantDiscoveryPublisher(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    SensorManager& sensorManager)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , sensorManager_(sensorManager) {
}

String HomeAssistantDiscoveryPublisher::stableDeviceId() const {
    char identifier[32];
    const uint64_t mac = ESP.getEfuseMac() & 0x0000FFFFFFFFFFFFULL;
    snprintf(identifier, sizeof(identifier), "weatherstation_%012llx",
        static_cast<unsigned long long>(mac));
    return String(identifier);
}

String HomeAssistantDiscoveryPublisher::discoveryTopic() const {
    return "homeassistant/device/" + stableDeviceId() + "/config";
}

uint32_t HomeAssistantDiscoveryPublisher::discoverySignature(uint16_t* componentMasks) const {
    for (size_t index = 0; index < MaxSensorCount; ++index) componentMasks[index] = 0;
    uint32_t hash = 2166136261UL;
    const Configuration& configuration = configurationService_.getConfiguration();
    hashText(hash, configuration.device.name.c_str());
    hashByte(hash, static_cast<uint8_t>(configuration.presentation.temperature));
    hashByte(hash, static_cast<uint8_t>(configuration.presentation.atmosphericPressure));
    hashByte(hash, static_cast<uint8_t>(configuration.presentation.solarCellTemperature));
    hashByte(hash, static_cast<uint8_t>(configuration.presentation.rainDetectorLevel));

    for (size_t index = 0; index < sensorManager_.sensorCount(); ++index) {
        SensorRuntimeInfo info;
        if (!sensorManager_.runtimeInfo(index, info)
            || !isValidSensorId(info.id)
            || info.id > MaxSensorCount) continue;
        hashByte(hash, static_cast<uint8_t>(info.id & 0xFF));
        hashByte(hash, static_cast<uint8_t>(info.id >> 8));
        hashText(hash, info.name);
        for (MeasurementType type : DiscoverableMeasurementTypes) {
            if (!supports(info, type)) continue;
            const uint8_t bit = static_cast<uint8_t>(type) - 1;
            componentMasks[info.id - 1] |= static_cast<uint16_t>(1U << bit);
            hashByte(hash, static_cast<uint8_t>(type));
        }
    }
    return hash;
}

String HomeAssistantDiscoveryPublisher::buildPayload(
    const uint16_t* componentMasks,
    const uint16_t* removalMasks,
    size_t& entityCount) const {
    const Configuration& configuration = configurationService_.getConfiguration();
    const String deviceId = stableDeviceId();
    const String stateRoot = "weatherstation/" + mqttTopicSafeDeviceName(configuration.device.name);
    String payload;
    payload.reserve(12288);
    payload = "{\"dev\":{\"ids\":[";
    appendJsonString(payload, deviceId.c_str());
    payload += "],\"name\":";
    appendJsonString(payload, configuration.device.name.c_str());
    payload += ",\"mf\":\"WeatherStation Project\",\"mdl\":\"WeatherStation\",\"sw\":";
    appendJsonString(payload, FirmwareVersion);
    payload += ",\"sn\":";
    appendJsonString(payload, deviceId.c_str());
    payload += "},\"o\":{\"name\":\"WeatherStation\",\"sw\":";
    appendJsonString(payload, FirmwareVersion);
    payload += "},\"cmps\":{";

    bool firstComponent = true;
    entityCount = 0;
    for (size_t slotIndex = 0; slotIndex < MaxSensorCount; ++slotIndex) {
        SensorRuntimeInfo runtime;
        bool hasRuntime = false;
        for (size_t runtimeIndex = 0; runtimeIndex < sensorManager_.sensorCount(); ++runtimeIndex) {
            SensorRuntimeInfo candidate;
            if (sensorManager_.runtimeInfo(runtimeIndex, candidate)
                && candidate.id == slotIndex + 1) {
                runtime = candidate;
                hasRuntime = true;
                break;
            }
        }
        for (MeasurementType type : DiscoverableMeasurementTypes) {
            const uint8_t bit = static_cast<uint8_t>(type) - 1;
            const uint16_t bitMask = static_cast<uint16_t>(1U << bit);
            const bool current = (componentMasks[slotIndex] & bitMask) != 0;
            const bool remove = removalMasks != nullptr
                && (removalMasks[slotIndex] & bitMask) != 0;
            if (!current && !remove) continue;
            const char* typeTopic = mqttMeasurementTypeTopic(type);
            if (typeTopic == nullptr) continue;
            const String uniqueId = deviceId + "_sensor_" + String(slotIndex + 1) + "_" + typeTopic;
            if (!firstComponent) payload += ',';
            firstComponent = false;
            appendJsonString(payload, uniqueId.c_str());
            payload += ":{\"p\":";
            appendJsonString(payload, entityPlatform(type));
            if (!current) {
                payload += '}';
                continue;
            }
            ++entityCount;
            payload += ",\"en\":true,\"unique_id\":";
            appendJsonString(payload, uniqueId.c_str());
            payload += ",\"name\":";
            const String entityName = String(hasRuntime ? runtime.name : "Sensor") + " "
                + measurementTypeMetadata(type).displayName;
            appendJsonString(payload, entityName.c_str());
            payload += ",\"state_topic\":";
            const String stateTopic = stateRoot + "/sensor/" + String(slotIndex + 1) + "/" + typeTopic;
            appendJsonString(payload, stateTopic.c_str());

            if (type == MeasurementType::RainGaugeTip) {
                payload += ",\"event_types\":[\"tip\"],\"value_template\":\"{\\\"event_type\\\":\\\"tip\\\"}\"";
            } else if (type == MeasurementType::RainDetectorWet) {
                payload += ",\"value_template\":\"{{ 'ON' if value_json.value else 'OFF' }}\"";
            } else {
                payload += ",\"value_template\":\"{{ value_json.value }}\"";
            }
            const char* mappedDeviceClass = deviceClass(type);
            if (mappedDeviceClass != nullptr) {
                payload += ",\"device_class\":";
                appendJsonString(payload, mappedDeviceClass);
            }
            if (hasMeasurementStateClass(type)) payload += ",\"state_class\":\"measurement\"";
            if (type != MeasurementType::RainDetectorWet && type != MeasurementType::RainGaugeTip) {
                PresentationUnit unit = configuration.presentationUnitFor(type);
                if (!supportsPresentationUnit(type, unit)) unit = measurementTypeMetadata(type).defaultPresentationUnit;
                const char* symbol = UnitConverter::symbol(unit);
                if (symbol != nullptr) {
                    payload += ",\"unit_of_measurement\":";
                    appendJsonString(payload, symbol);
                }
            }
            payload += '}';
        }
    }
    payload += "}}";
    return payload;
}

bool HomeAssistantDiscoveryPublisher::publishPayload(const String& payload) {
    return mqttService_.publish(discoveryTopic().c_str(), payload.c_str(), true);
}

bool HomeAssistantDiscoveryPublisher::publishDiscovery(
    const uint16_t* componentMasks,
    bool logPublication) {
    uint16_t removals[MaxSensorCount];
    bool hasRemovals = false;
    for (size_t index = 0; index < MaxSensorCount; ++index) {
        removals[index] = publishedComponentMasks_[index] & ~componentMasks[index];
        hasRemovals = hasRemovals || removals[index] != 0;
    }
    if (hasRemovals) {
        size_t updateEntityCount = 0;
        const String removalPayload = buildPayload(componentMasks, removals, updateEntityCount);
        if (!publishPayload(removalPayload)) return false;
    }
    size_t entityCount = 0;
    const String payload = buildPayload(componentMasks, nullptr, entityCount);
    if (!publishPayload(payload)) return false;
    for (size_t index = 0; index < MaxSensorCount; ++index) {
        publishedComponentMasks_[index] = componentMasks[index];
    }
    lastPayloadSize_ = payload.length();
    lastEntityCount_ = entityCount;
    if (logPublication) {
        logger_.printf("Home Assistant discovery published: %u entities\n",
            static_cast<unsigned int>(entityCount));
    }
    return true;
}

DiscoveryRepublishResult HomeAssistantDiscoveryPublisher::republish() {
    if (!mqttService_.connected()) return DiscoveryRepublishResult::MqttUnavailable;

    uint16_t componentMasks[MaxSensorCount];
    const uint32_t signature = discoverySignature(componentMasks);
    if (!clearedAfterBoot_) {
        if (!publishPayload(String())) return DiscoveryRepublishResult::PublishFailed;
        clearedAfterBoot_ = true;
    }
    if (!publishDiscovery(componentMasks, false)) {
        return DiscoveryRepublishResult::PublishFailed;
    }
    publishedSignature_ = signature;
    wasConnected_ = true;
    logger_.printf("Home Assistant Discovery manually republished: %u entities\n",
        static_cast<unsigned int>(lastEntityCount_));
    return DiscoveryRepublishResult::Published;
}

void HomeAssistantDiscoveryPublisher::loop() {
    const bool connected = mqttService_.connected();
    if (!connected) {
        wasConnected_ = false;
        return;
    }
    const bool connectionEstablished = !wasConnected_;
    wasConnected_ = true;
    uint16_t componentMasks[MaxSensorCount];
    const uint32_t signature = discoverySignature(componentMasks);
    if (!connectionEstablished && signature == publishedSignature_) return;
    const uint32_t nowMs = millis();
    if (!connectionEstablished
        && static_cast<uint32_t>(nowMs - lastAttemptMs_) < RetryIntervalMs) return;
    lastAttemptMs_ = nowMs;

    if (!clearedAfterBoot_) {
        if (!publishPayload(String())) {
            logger_.println("Discovery publication failed");
            return;
        }
        clearedAfterBoot_ = true;
    }
    if (!publishDiscovery(componentMasks)) {
        logger_.println("Discovery publication failed");
        return;
    }
    if (publishedSignature_ != 0 && signature != publishedSignature_) {
        logger_.println("Discovery updated after Sensor composition change");
    }
    publishedSignature_ = signature;
}

size_t HomeAssistantDiscoveryPublisher::lastPayloadSize() const {
    return lastPayloadSize_;
}

size_t HomeAssistantDiscoveryPublisher::lastEntityCount() const {
    return lastEntityCount_;
}

} // namespace WeatherStation
