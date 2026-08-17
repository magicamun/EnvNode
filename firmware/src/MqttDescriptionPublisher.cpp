#include "MqttDescriptionPublisher.h"

#include <cstring>

#include "ExternalDescriptionBuilder.h"
#include "MqttTopic.h"

namespace EnvNode {
namespace {

constexpr uint32_t FnvOffset = 2166136261UL;
constexpr uint32_t FnvPrime = 16777619UL;

void hashBytes(uint32_t& hash, const void* data, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t index = 0; index < length; ++index) {
        hash ^= bytes[index];
        hash *= FnvPrime;
    }
}

template <typename T>
void hashValue(uint32_t& hash, const T& value) {
    hashBytes(hash, &value, sizeof(value));
}

void hashString(uint32_t& hash, const String& value) {
    const size_t length = value.length();
    hashValue(hash, length);
    hashBytes(hash, value.c_str(), length);
}

uint32_t actuatorSignature(const ActuatorSlotConfiguration& slot) {
    uint32_t hash = FnvOffset;
    hashValue(hash, slot.slotId);
    hashValue(hash, slot.enabled);
    hashString(hash, slot.name);
    hashValue(hash, slot.implementation);
    return hash;
}

uint32_t controllerSignature(const ControllerSlotConfiguration& slot) {
    uint32_t hash = FnvOffset;
    hashValue(hash, slot.slotId);
    hashValue(hash, slot.enabled);
    hashString(hash, slot.name);
    hashValue(hash, slot.implementation);
    switch (slot.implementation) {
        case ControllerImplementation::Blink: {
            const BlinkControllerConfiguration& blink =
                slot.implementationConfiguration.blink;
            hashValue(hash, blink.targetActuatorId);
            hashValue(hash, blink.onDurationMs);
            hashValue(hash, blink.offDurationMs);
            break;
        }
        case ControllerImplementation::Threshold: {
            const ThresholdControllerConfiguration& threshold =
                slot.implementationConfiguration.threshold;
            hashValue(hash, threshold.source.sensorId);
            hashValue(hash, threshold.source.measurementType);
            hashValue(hash, threshold.targetActuatorId);
            hashValue(hash, threshold.onThreshold);
            hashValue(hash, threshold.offThreshold);
            hashValue(hash, threshold.maxMeasurementAgeMs);
            break;
        }
        case ControllerImplementation::None:
        default:
            break;
    }
    return hash;
}

bool deadlineReached(uint32_t now, uint32_t deadline) {
    return static_cast<int32_t>(now - deadline) >= 0;
}

} // namespace

MqttDescriptionPublisher::MqttDescriptionPublisher(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    IMonotonicClock& clock)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , clock_(clock) {
}

void MqttDescriptionPublisher::beginReconciliation() {
    memset(actuatorSynchronized_, 0, sizeof(actuatorSynchronized_));
    memset(controllerSynchronized_, 0, sizeof(controllerSynchronized_));
    reconciliationActive_ = true;
    failureReported_ = false;
    retryAtMs_ = 0;
    logger_.debug("MQTT description reconciliation started");
}

bool MqttDescriptionPublisher::synchronizeActuator(
    size_t index,
    const Configuration& configuration) {
    const ActuatorSlotConfiguration& slot = configuration.actuatorSlots[index];
    const uint32_t signature = actuatorSignature(slot);
    if (actuatorSynchronized_[index]
        && actuatorSignatures_[index] == signature) return true;

    String payload;
    buildActuatorDescription(slot, payload);
    const ActuatorId id = static_cast<ActuatorId>(index + 1);
    const String topic = mqttActuatorDescriptionTopic(configuration.device.name, id);
    if (!mqttService_.publish(topic.c_str(), payload.c_str(), true)) return false;
    actuatorSignatures_[index] = signature;
    actuatorSynchronized_[index] = true;
    return true;
}

bool MqttDescriptionPublisher::synchronizeController(
    size_t index,
    const Configuration& configuration) {
    const ControllerSlotConfiguration& slot = configuration.controllerSlots[index];
    const uint32_t signature = controllerSignature(slot);
    if (controllerSynchronized_[index]
        && controllerSignatures_[index] == signature) return true;

    String payload;
    buildControllerDescription(slot, payload);
    const ControllerId id = static_cast<ControllerId>(index + 1);
    const String topic = mqttControllerDescriptionTopic(configuration.device.name, id);
    if (!mqttService_.publish(topic.c_str(), payload.c_str(), true)) return false;
    controllerSignatures_[index] = signature;
    controllerSynchronized_[index] = true;
    return true;
}

void MqttDescriptionPublisher::loop() {
    if (!mqttService_.connected()) {
        wasConnected_ = false;
        return;
    }
    if (!wasConnected_) beginReconciliation();
    wasConnected_ = true;

    const uint32_t now = clock_.nowMs();
    if (failureReported_ && !deadlineReached(now, retryAtMs_)) return;

    const Configuration& configuration = configurationService_.getConfiguration();
    bool success = true;
    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        if (!synchronizeActuator(index, configuration)) success = false;
    }
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        if (!synchronizeController(index, configuration)) success = false;
    }

    if (!success) {
        if (!failureReported_) logger_.warn("MQTT description publication failed; retry scheduled");
        failureReported_ = true;
        retryAtMs_ = now + RetryDelayMs;
        return;
    }
    if (reconciliationActive_) {
        logger_.debugf("MQTT description reconciliation completed: %u actuator and %u controller slots synchronized",
            static_cast<unsigned int>(MaxActuatorSlotCount),
            static_cast<unsigned int>(MaxControllerSlotCount));
    } else if (failureReported_) {
        logger_.info("MQTT description publication recovered");
    }
    reconciliationActive_ = false;
    failureReported_ = false;
}

} // namespace EnvNode
