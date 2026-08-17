#include "ControllerStatePublisher.h"

#include "MqttTopic.h"

namespace EnvNode {
namespace {

const char* phaseName(BlinkPhase phase) {
    switch (phase) {
        case BlinkPhase::WaitingForTarget: return "waiting_for_target";
        case BlinkPhase::On: return "on";
        case BlinkPhase::Off: return "off";
        case BlinkPhase::Stopped: return "stopped";
        default: return "stopped";
    }
}

const char* resultName(ControllerOperationResult result) {
    switch (result) {
        case ControllerOperationResult::Completed: return "completed";
        case ControllerOperationResult::NoAction: return "no_action";
        case ControllerOperationResult::TargetUnavailable: return "target_unavailable";
        case ControllerOperationResult::ActuatorOperationFailed: return "actuator_operation_failed";
        case ControllerOperationResult::InvalidConfiguration: return "invalid_configuration";
        case ControllerOperationResult::NotRunning: return "not_running";
        case ControllerOperationResult::ControllerNotFound: return "controller_not_found";
        default: return "unknown";
    }
}

} // namespace

ControllerStatePublisher::ControllerStatePublisher(
    ILogger& logger,
    IConfigurationService& configurationService,
    IMqttService& mqttService,
    ControllerRuntime& controllerRuntime)
    : logger_(logger)
    , configurationService_(configurationService)
    , mqttService_(mqttService)
    , controllerRuntime_(controllerRuntime) {
}

void ControllerStatePublisher::loop() {
    if (!mqttService_.connected()) {
        wasConnected_ = false;
        return;
    }
    const uint32_t compositionRevision = controllerRuntime_.compositionRevision();
    const bool forcePublish = !wasConnected_
        || observedCompositionRevision_ != compositionRevision;
    wasConnected_ = true;
    observedCompositionRevision_ = compositionRevision;
    const Configuration& configuration = configurationService_.getConfiguration();

    bool active[MaxControllerSlotCount] = {};
    for (size_t index = 0; index < controllerRuntime_.runtimeCount(); ++index) {
        ControllerRuntimeInfo info;
        if (!controllerRuntime_.runtimeInfo(index, info)
            || !isValidControllerId(info.id)
            || info.id > MaxControllerSlotCount) continue;
        const size_t slotIndex = info.id - 1;
        active[slotIndex] = true;
        StatusSnapshot current;
        current.running = info.running;
        current.targetAvailable = info.targetAvailable;
        current.phase = info.blinkPhase;
        current.lastResult = info.lastOperationResult;
        if (!forcePublish && statusKnown_[slotIndex]
            && sameStatus(statuses_[slotIndex], current)) continue;
        const String topic = mqttControllerStatusTopic(configuration.device.name, info.id);
        const String payload = statusPayload(info);
        if (mqttService_.publish(topic.c_str(), payload.c_str(), true)) {
            statusKnown_[slotIndex] = true;
            statuses_[slotIndex] = current;
        } else {
            logger_.printf("MQTT Controller %u status publish failed\n",
                static_cast<unsigned int>(info.id));
        }
    }
    for (size_t index = 0; index < MaxControllerSlotCount; ++index) {
        const ControllerId id = static_cast<ControllerId>(index + 1);
        if (statusKnown_[index] && !active[index]) {
            const String topic = mqttControllerStatusTopic(configuration.device.name, id);
            if (mqttService_.publish(topic.c_str(), "", true)) statusKnown_[index] = false;
        }

        const ControllerSlotConfiguration& slot = configuration.controllerSlots[index];
        const bool hasParameters = slot.implementation == ControllerImplementation::Blink;
        if (hasParameters) {
            const uint32_t on = slot.implementationConfiguration.blink.onDurationMs;
            const uint32_t off = slot.implementationConfiguration.blink.offDurationMs;
            if (forcePublish || !onParametersKnown_[index] || onDurations_[index] != on) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, ControllerMqttParameter::OnDurationMs);
                const String payload(on);
                if (mqttService_.publish(topic.c_str(), payload.c_str(), true)) {
                    onDurations_[index] = on;
                    onParametersKnown_[index] = true;
                }
            }
            if (forcePublish || !offParametersKnown_[index] || offDurations_[index] != off) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, ControllerMqttParameter::OffDurationMs);
                const String payload(off);
                if (mqttService_.publish(topic.c_str(), payload.c_str(), true)) {
                    offDurations_[index] = off;
                    offParametersKnown_[index] = true;
                }
            }
        } else {
            if (onParametersKnown_[index]) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, ControllerMqttParameter::OnDurationMs);
                if (mqttService_.publish(topic.c_str(), "", true)) {
                    onParametersKnown_[index] = false;
                }
            }
            if (offParametersKnown_[index]) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, ControllerMqttParameter::OffDurationMs);
                if (mqttService_.publish(topic.c_str(), "", true)) {
                    offParametersKnown_[index] = false;
                }
            }
        }
    }
}

bool ControllerStatePublisher::sameStatus(
    const StatusSnapshot& left,
    const StatusSnapshot& right) {
    return left.running == right.running
        && left.targetAvailable == right.targetAvailable
        && left.phase == right.phase
        && left.lastResult == right.lastResult;
}

String ControllerStatePublisher::statusPayload(const ControllerRuntimeInfo& info) {
    return String("{\"running\":") + (info.running ? "true" : "false")
        + ",\"phase\":\"" + phaseName(info.blinkPhase)
        + "\",\"target_available\":" + (info.targetAvailable ? "true" : "false")
        + ",\"last_result\":\"" + resultName(info.lastOperationResult) + "\"}";
}

} // namespace EnvNode
