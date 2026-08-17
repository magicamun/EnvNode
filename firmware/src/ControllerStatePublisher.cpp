#include "ControllerStatePublisher.h"

#include <cstdio>

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

const char* decisionName(ThresholdDecision decision) {
    switch (decision) {
        case ThresholdDecision::On: return "on";
        case ThresholdDecision::Off: return "off";
        case ThresholdDecision::Unknown:
        default: return "unknown";
    }
}

String compactFloat(float value) {
    char buffer[24];
    snprintf(buffer, sizeof(buffer), "%.9g", static_cast<double>(value));
    return String(buffer);
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
        current.implementation = info.implementation;
        current.running = info.running;
        current.targetAvailable = info.targetAvailable;
        current.phase = info.blinkPhase;
        current.sourceAvailable = info.sourceAvailable;
        current.measurementValid = info.latestMeasurementValid;
        current.stale = info.latestSnapshotStale;
        current.decision = info.thresholdDecision;
        current.outputPending = info.outputApplicationPending;
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
        for (uint8_t parameterValueIndex = 0;
             parameterValueIndex < static_cast<uint8_t>(ControllerParameter::Count);
             ++parameterValueIndex) {
            const ControllerParameter parameter =
                static_cast<ControllerParameter>(parameterValueIndex);
            String value;
            const bool supported = parameterValue(slot, parameter, value);
            if (supported
                && (forcePublish
                    || !parameterKnown_[index][parameterValueIndex]
                    || parameterValues_[index][parameterValueIndex] != value)) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, parameter);
                if (mqttService_.publish(topic.c_str(), value.c_str(), true)) {
                    parameterValues_[index][parameterValueIndex] = value;
                    parameterKnown_[index][parameterValueIndex] = true;
                }
            } else if (!supported && parameterKnown_[index][parameterValueIndex]) {
                const String topic = mqttControllerParameterTopic(
                    configuration.device.name, id, parameter);
                if (mqttService_.publish(topic.c_str(), "", true)) {
                    parameterKnown_[index][parameterValueIndex] = false;
                    parameterValues_[index][parameterValueIndex] = String();
                }
            }
        }
    }
}

bool ControllerStatePublisher::sameStatus(
    const StatusSnapshot& left,
    const StatusSnapshot& right) {
    return left.implementation == right.implementation
        && left.running == right.running
        && left.targetAvailable == right.targetAvailable
        && left.phase == right.phase
        && left.sourceAvailable == right.sourceAvailable
        && left.measurementValid == right.measurementValid
        && left.stale == right.stale
        && left.decision == right.decision
        && left.outputPending == right.outputPending
        && left.lastResult == right.lastResult;
}

String ControllerStatePublisher::statusPayload(const ControllerRuntimeInfo& info) {
    if (info.implementation == ControllerImplementation::Threshold) {
        return String("{\"running\":") + (info.running ? "true" : "false")
            + ",\"source_available\":" + (info.sourceAvailable ? "true" : "false")
            + ",\"measurement_valid\":" + (info.latestMeasurementValid ? "true" : "false")
            + ",\"stale\":" + (info.latestSnapshotStale ? "true" : "false")
            + ",\"decision\":\"" + decisionName(info.thresholdDecision)
            + "\",\"target_available\":" + (info.targetAvailable ? "true" : "false")
            + ",\"output_pending\":" + (info.outputApplicationPending ? "true" : "false")
            + ",\"last_result\":\"" + resultName(info.lastOperationResult) + "\"}";
    }
    return String("{\"running\":") + (info.running ? "true" : "false")
        + ",\"phase\":\"" + phaseName(info.blinkPhase)
        + "\",\"target_available\":" + (info.targetAvailable ? "true" : "false")
        + ",\"last_result\":\"" + resultName(info.lastOperationResult) + "\"}";
}

bool ControllerStatePublisher::parameterValue(
    const ControllerSlotConfiguration& slot,
    ControllerParameter parameter,
    String& value) {
    if (slot.implementation == ControllerImplementation::Blink) {
        if (parameter == ControllerParameter::OnDurationMs) {
            value = String(slot.implementationConfiguration.blink.onDurationMs);
            return true;
        }
        if (parameter == ControllerParameter::OffDurationMs) {
            value = String(slot.implementationConfiguration.blink.offDurationMs);
            return true;
        }
        return false;
    }
    if (slot.implementation == ControllerImplementation::Threshold) {
        const ThresholdControllerConfiguration& threshold =
            slot.implementationConfiguration.threshold;
        if (parameter == ControllerParameter::OnThreshold) {
            value = compactFloat(threshold.onThreshold);
            return true;
        }
        if (parameter == ControllerParameter::OffThreshold) {
            value = compactFloat(threshold.offThreshold);
            return true;
        }
        if (parameter == ControllerParameter::MaxMeasurementAgeMs) {
            value = String(threshold.maxMeasurementAgeMs);
            return true;
        }
    }
    return false;
}

} // namespace EnvNode
