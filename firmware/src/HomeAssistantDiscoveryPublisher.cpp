#include "HomeAssistantDiscoveryPublisher.h"

#include <Arduino.h>

#include "FirmwareVersion.h"
#include "JsonWriter.h"
#include "MqttTopic.h"
#include "UnitConverter.h"

namespace EnvNode {
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

enum ActuatorDiscoveryPlatform : uint8_t {
    NoActuatorPlatform = 0,
    SwitchActuatorPlatform = 1,
    LightActuatorPlatform = 2,
};

uint8_t actuatorDiscoveryPlatform(ActuatorCapability capabilities) {
    if (hasActuatorCapability(capabilities, ActuatorCapability::Level)) {
        return LightActuatorPlatform;
    }
    if (hasActuatorCapability(capabilities, ActuatorCapability::OnOff)) {
        return SwitchActuatorPlatform;
    }
    return NoActuatorPlatform;
}

const char* actuatorPlatformName(uint8_t platform) {
    switch (platform) {
        case SwitchActuatorPlatform: return "switch";
        case LightActuatorPlatform: return "light";
        default: return nullptr;
    }
}

} // namespace

HomeAssistantDiscoveryPublisher::HomeAssistantDiscoveryPublisher(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    SensorManager& sensorManager,
    ActuatorRuntime& actuatorRuntime)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , sensorManager_(sensorManager)
    , actuatorRuntime_(actuatorRuntime) {
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

uint32_t HomeAssistantDiscoveryPublisher::discoverySignature(
    uint16_t* sensorComponentMasks,
    uint8_t* actuatorPlatforms) const {
    for (size_t index = 0; index < MaxSensorCount; ++index) sensorComponentMasks[index] = 0;
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) actuatorPlatforms[index] = 0;
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
            sensorComponentMasks[info.id - 1] |= static_cast<uint16_t>(1U << bit);
            hashByte(hash, static_cast<uint8_t>(type));
        }
    }
    for (size_t index = 0; index < actuatorRuntime_.runtimeCount(); ++index) {
        ActuatorRuntimeInfo info;
        if (!actuatorRuntime_.runtimeInfo(index, info)
            || !isValidActuatorId(info.id)
            || info.id > MaxActuatorSlotCount) continue;
        const uint8_t platform = actuatorDiscoveryPlatform(info.capabilities);
        if (platform == NoActuatorPlatform) continue;
        actuatorPlatforms[info.id - 1] = platform;
        hashByte(hash, static_cast<uint8_t>(info.id & 0xFF));
        hashByte(hash, static_cast<uint8_t>(info.id >> 8));
        hashText(hash, info.name);
        hashByte(hash, platform);
    }
    return hash;
}

String HomeAssistantDiscoveryPublisher::buildPayload(
    const uint16_t* sensorComponentMasks,
    const uint16_t* sensorRemovalMasks,
    const uint8_t* actuatorPlatforms,
    const uint8_t* actuatorRemovalPlatforms,
    size_t& entityCount) const {
    const Configuration& configuration = configurationService_.getConfiguration();
    const String deviceId = stableDeviceId();
    const String stateRoot = mqttDeviceTopicRoot(configuration.device.name);
    String payload;
    payload.reserve(12288);
    payload = "{\"dev\":{\"ids\":[";
    appendJsonString(payload, deviceId.c_str());
    payload += "],\"name\":";
    appendJsonString(payload, configuration.device.name.c_str());
    payload += ",\"mf\":\"EnvNode Project\",\"mdl\":\"EnvNode\",\"sw\":";
    appendJsonString(payload, FirmwareVersion);
    payload += ",\"sn\":";
    appendJsonString(payload, deviceId.c_str());
    payload += "},\"o\":{\"name\":\"EnvNode\",\"sw\":";
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
            const bool current = (sensorComponentMasks[slotIndex] & bitMask) != 0;
            const bool remove = sensorRemovalMasks != nullptr
                && (sensorRemovalMasks[slotIndex] & bitMask) != 0;
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
    for (size_t slotIndex = 0; slotIndex < MaxActuatorSlotCount; ++slotIndex) {
        const uint8_t platform = actuatorPlatforms[slotIndex];
        const uint8_t removalPlatform = actuatorRemovalPlatforms == nullptr
            ? NoActuatorPlatform : actuatorRemovalPlatforms[slotIndex];
        const uint8_t emittedPlatform = removalPlatform != NoActuatorPlatform
            ? removalPlatform : platform;
        const char* platformName = actuatorPlatformName(emittedPlatform);
        if (platformName == nullptr) continue;

        ActuatorRuntimeInfo runtime;
        bool hasRuntime = false;
        for (size_t runtimeIndex = 0;
             runtimeIndex < actuatorRuntime_.runtimeCount();
             ++runtimeIndex) {
            ActuatorRuntimeInfo candidate;
            if (actuatorRuntime_.runtimeInfo(runtimeIndex, candidate)
                && candidate.id == slotIndex + 1) {
                runtime = candidate;
                hasRuntime = true;
                break;
            }
        }

        const String uniqueId = deviceId + "_actuator_" + String(slotIndex + 1);
        if (!firstComponent) payload += ',';
        firstComponent = false;
        appendJsonString(payload, uniqueId.c_str());
        payload += ":{\"p\":";
        appendJsonString(payload, platformName);
        if (platform == NoActuatorPlatform || removalPlatform != NoActuatorPlatform) {
            payload += '}';
            continue;
        }

        ++entityCount;
        const ActuatorId id = static_cast<ActuatorId>(slotIndex + 1);
        payload += ",\"en\":true,\"unique_id\":";
        appendJsonString(payload, uniqueId.c_str());
        payload += ",\"name\":";
        appendJsonString(payload, hasRuntime ? runtime.name : "Actuator");
        payload += ",\"command_topic\":";
        const String commandTopic = mqttActuatorCommandTopic(configuration.device.name, id);
        appendJsonString(payload, commandTopic.c_str());
        payload += ",\"state_topic\":";
        const String statusTopic = mqttActuatorStatusTopic(configuration.device.name, id);
        appendJsonString(payload, statusTopic.c_str());
        payload += ",\"payload_on\":\"ON\",\"payload_off\":\"OFF\"";
        if (platform == LightActuatorPlatform) {
            payload += ",\"on_command_type\":\"first\"";
            payload += ",\"brightness_command_topic\":";
            const String levelCommandTopic = mqttActuatorLevelCommandTopic(
                configuration.device.name, id);
            appendJsonString(payload, levelCommandTopic.c_str());
            payload += ",\"brightness_state_topic\":";
            const String levelStatusTopic = mqttActuatorLevelStatusTopic(
                configuration.device.name, id);
            appendJsonString(payload, levelStatusTopic.c_str());
            payload += ",\"brightness_scale\":100";
        }
        payload += '}';
    }
    payload += "}}";
    return payload;
}

bool HomeAssistantDiscoveryPublisher::publishPayload(const String& payload) {
    return mqttService_.publish(discoveryTopic().c_str(), payload.c_str(), true);
}

bool HomeAssistantDiscoveryPublisher::publishDiscovery(
    const uint16_t* sensorComponentMasks,
    const uint8_t* actuatorPlatforms,
    bool logPublication) {
    uint16_t sensorRemovals[MaxSensorCount];
    uint8_t actuatorRemovals[MaxActuatorSlotCount];
    bool hasRemovals = false;
    for (size_t index = 0; index < MaxSensorCount; ++index) {
        sensorRemovals[index] = publishedComponentMasks_[index] & ~sensorComponentMasks[index];
        hasRemovals = hasRemovals || sensorRemovals[index] != 0;
    }
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        actuatorRemovals[index] = publishedActuatorPlatforms_[index] != NoActuatorPlatform
                && publishedActuatorPlatforms_[index] != actuatorPlatforms[index]
            ? publishedActuatorPlatforms_[index] : NoActuatorPlatform;
        hasRemovals = hasRemovals || actuatorRemovals[index] != NoActuatorPlatform;
    }
    if (hasRemovals) {
        size_t updateEntityCount = 0;
        const String removalPayload = buildPayload(
            sensorComponentMasks,
            sensorRemovals,
            actuatorPlatforms,
            actuatorRemovals,
            updateEntityCount);
        if (!publishPayload(removalPayload)) return false;
    }
    size_t entityCount = 0;
    const String payload = buildPayload(
        sensorComponentMasks, nullptr, actuatorPlatforms, nullptr, entityCount);
    if (!publishPayload(payload)) return false;
    for (size_t index = 0; index < MaxSensorCount; ++index) {
        publishedComponentMasks_[index] = sensorComponentMasks[index];
    }
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        publishedActuatorPlatforms_[index] = actuatorPlatforms[index];
    }
    lastPayloadSize_ = payload.length();
    lastEntityCount_ = entityCount;
    if (logPublication) {
        logger_.infof("Home Assistant discovery published: %u entities",
            static_cast<unsigned int>(entityCount));
    }
    return true;
}

DiscoveryRepublishResult HomeAssistantDiscoveryPublisher::republish() {
    if (!mqttService_.connected()) return DiscoveryRepublishResult::MqttUnavailable;

    uint16_t sensorComponentMasks[MaxSensorCount];
    uint8_t actuatorPlatforms[MaxActuatorSlotCount];
    const uint32_t signature = discoverySignature(sensorComponentMasks, actuatorPlatforms);
    if (!clearedAfterBoot_) {
        if (!publishPayload(String())) return DiscoveryRepublishResult::PublishFailed;
        clearedAfterBoot_ = true;
    }
    if (!publishDiscovery(sensorComponentMasks, actuatorPlatforms, false)) {
        return DiscoveryRepublishResult::PublishFailed;
    }
    publishedSignature_ = signature;
    wasConnected_ = true;
    logger_.infof("Home Assistant Discovery manually republished: %u entities",
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
    uint16_t sensorComponentMasks[MaxSensorCount];
    uint8_t actuatorPlatforms[MaxActuatorSlotCount];
    const uint32_t signature = discoverySignature(sensorComponentMasks, actuatorPlatforms);
    if (!connectionEstablished && signature == publishedSignature_) return;
    const uint32_t nowMs = millis();
    if (!connectionEstablished
        && static_cast<uint32_t>(nowMs - lastAttemptMs_) < RetryIntervalMs) return;
    lastAttemptMs_ = nowMs;

    if (!clearedAfterBoot_) {
        if (!publishPayload(String())) {
            if (!publicationFailureReported_) {
                logger_.warn("Discovery publication failed; retry pending");
                publicationFailureReported_ = true;
            }
            return;
        }
        clearedAfterBoot_ = true;
    }
    if (!publishDiscovery(sensorComponentMasks, actuatorPlatforms)) {
        if (!publicationFailureReported_) {
            logger_.warn("Discovery publication failed; retry pending");
            publicationFailureReported_ = true;
        }
        return;
    }
    if (publicationFailureReported_) {
        logger_.info("Discovery publication recovered");
        publicationFailureReported_ = false;
    }
    if (publishedSignature_ != 0 && signature != publishedSignature_) {
        logger_.info("Discovery updated after runtime composition change");
    }
    publishedSignature_ = signature;
}

size_t HomeAssistantDiscoveryPublisher::lastPayloadSize() const {
    return lastPayloadSize_;
}

size_t HomeAssistantDiscoveryPublisher::lastEntityCount() const {
    return lastEntityCount_;
}

} // namespace EnvNode
