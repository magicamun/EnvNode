#include "ActuatorStatePublisher.h"

#include "MqttTopic.h"

namespace EnvNode {

ActuatorStatePublisher::ActuatorStatePublisher(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    ActuatorRuntime& actuatorRuntime)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , actuatorRuntime_(actuatorRuntime) {
}

void ActuatorStatePublisher::loop() {
    const bool connected = mqttService_.connected();
    if (!connected) {
        wasConnected_ = false;
        return;
    }
    const bool forcePublish = !wasConnected_;
    wasConnected_ = true;

    bool active[MaxActuatorSlotCount] = {};
    bool levelActive[MaxActuatorSlotCount] = {};
    for (size_t index = 0; index < actuatorRuntime_.runtimeCount(); ++index) {
        ActuatorRuntimeInfo info;
        if (!actuatorRuntime_.runtimeInfo(index, info)
            || info.id == InvalidActuatorId
            || info.id > MaxActuatorSlotCount) {
            continue;
        }
        const size_t slotIndex = static_cast<size_t>(info.id - 1);
        const IOnOffActuator* actuator = actuatorRuntime_.onOffActuator(info.id);
        if (actuator == nullptr) continue;
        active[slotIndex] = true;
        const OnOffState state = actuator->state();
        if (forcePublish || !known_[slotIndex] || states_[slotIndex] != state) {
            const String topic = mqttActuatorStatusTopic(
                configurationService_.getConfiguration().device.name, info.id);
            if (mqttService_.publish(
                    topic.c_str(), state == OnOffState::On ? "ON" : "OFF", true)) {
                known_[slotIndex] = true;
                states_[slotIndex] = state;
                if (publicationFailureReported_[slotIndex]) {
                    logger_.infof("MQTT actuator %u status publication recovered",
                        static_cast<unsigned int>(info.id));
                    publicationFailureReported_[slotIndex] = false;
                }
            } else if (!publicationFailureReported_[slotIndex]) {
                logger_.warnf("MQTT actuator %u status publish failed; retry pending",
                    static_cast<unsigned int>(info.id));
                publicationFailureReported_[slotIndex] = true;
            }
        }

        const ILevelActuator* levelActuator = actuatorRuntime_.levelActuator(info.id);
        if (levelActuator == nullptr) continue;
        levelActive[slotIndex] = true;
        const ActuatorLevel level = levelActuator->level();
        if (!forcePublish && levelKnown_[slotIndex] && levels_[slotIndex] == level) continue;
        const String levelTopic = mqttActuatorLevelStatusTopic(
            configurationService_.getConfiguration().device.name, info.id);
        const String levelPayload(static_cast<unsigned int>(level.percent()));
        if (mqttService_.publish(levelTopic.c_str(), levelPayload.c_str(), true)) {
            levelKnown_[slotIndex] = true;
            levels_[slotIndex] = level;
            if (levelPublicationFailureReported_[slotIndex]) {
                logger_.infof("MQTT actuator %u Level publication recovered",
                    static_cast<unsigned int>(info.id));
                levelPublicationFailureReported_[slotIndex] = false;
            }
        } else if (!levelPublicationFailureReported_[slotIndex]) {
            logger_.warnf("MQTT actuator %u Level publish failed; retry pending",
                static_cast<unsigned int>(info.id));
            levelPublicationFailureReported_[slotIndex] = true;
        }
    }

    for (size_t index = 0; index < MaxActuatorSlotCount; ++index) {
        if (!active[index]) publicationFailureReported_[index] = false;
        if (!levelActive[index]) levelPublicationFailureReported_[index] = false;
        const ActuatorId id = static_cast<ActuatorId>(index + 1);
        if (known_[index] && !active[index]) {
            const String topic = mqttActuatorStatusTopic(
                configurationService_.getConfiguration().device.name, id);
            if (mqttService_.publish(topic.c_str(), "", true)) {
                known_[index] = false;
            }
        }
        if (levelKnown_[index] && !levelActive[index]) {
            const String levelTopic = mqttActuatorLevelStatusTopic(
                configurationService_.getConfiguration().device.name, id);
            if (mqttService_.publish(levelTopic.c_str(), "", true)) {
                levelKnown_[index] = false;
            }
        }
    }
}

} // namespace EnvNode
