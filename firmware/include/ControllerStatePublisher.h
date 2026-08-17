#pragma once

#include "ControllerRuntime.h"
#include "IConfigurationService.h"
#include "IMqttService.h"
#include "Logger.h"
#include "MqttTopic.h"

namespace EnvNode {

class ControllerStatePublisher {
public:
    ControllerStatePublisher(
        ILogger& logger,
        IConfigurationService& configurationService,
        IMqttService& mqttService,
        ControllerRuntime& controllerRuntime);

    void loop();

private:
    struct StatusSnapshot {
        ControllerImplementation implementation = ControllerImplementation::None;
        bool running = false;
        bool targetAvailable = false;
        BlinkPhase phase = BlinkPhase::Stopped;
        bool sourceAvailable = false;
        bool measurementValid = false;
        bool stale = false;
        ThresholdDecision decision = ThresholdDecision::Unknown;
        bool outputPending = false;
        ControllerOperationResult lastResult = ControllerOperationResult::NotRunning;
    };

    static bool sameStatus(const StatusSnapshot& left, const StatusSnapshot& right);
    static String statusPayload(const ControllerRuntimeInfo& info);
    static bool parameterValue(
        const ControllerSlotConfiguration& slot,
        ControllerMqttParameter parameter,
        String& value);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    ControllerRuntime& controllerRuntime_;
    bool wasConnected_ = false;
    uint32_t observedCompositionRevision_ = 0;
    bool statusKnown_[MaxControllerSlotCount] = {};
    StatusSnapshot statuses_[MaxControllerSlotCount];
    bool parameterKnown_[MaxControllerSlotCount]
        [static_cast<size_t>(ControllerMqttParameter::Count)] = {};
    String parameterValues_[MaxControllerSlotCount]
        [static_cast<size_t>(ControllerMqttParameter::Count)];
};

} // namespace EnvNode
