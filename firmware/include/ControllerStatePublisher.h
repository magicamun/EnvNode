#pragma once

#include "ControllerRuntime.h"
#include "IConfigurationService.h"
#include "IMqttService.h"
#include "Logger.h"

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
        bool running = false;
        bool targetAvailable = false;
        BlinkPhase phase = BlinkPhase::Stopped;
        ControllerOperationResult lastResult = ControllerOperationResult::NotRunning;
    };

    static bool sameStatus(const StatusSnapshot& left, const StatusSnapshot& right);
    static String statusPayload(const ControllerRuntimeInfo& info);

    ILogger& logger_;
    IConfigurationService& configurationService_;
    IMqttService& mqttService_;
    ControllerRuntime& controllerRuntime_;
    bool wasConnected_ = false;
    uint32_t observedCompositionRevision_ = 0;
    bool statusKnown_[MaxControllerSlotCount] = {};
    StatusSnapshot statuses_[MaxControllerSlotCount];
    bool onParametersKnown_[MaxControllerSlotCount] = {};
    bool offParametersKnown_[MaxControllerSlotCount] = {};
    uint32_t onDurations_[MaxControllerSlotCount] = {};
    uint32_t offDurations_[MaxControllerSlotCount] = {};
};

} // namespace EnvNode
