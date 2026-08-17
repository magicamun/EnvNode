#include "RuntimeManager.h"

#include <Arduino.h>

namespace EnvNode {

const char* runtimeActionName(RuntimeAction action) {
    switch (action) {
        case RuntimeAction::None: return "None";
        case RuntimeAction::RestartMqtt: return "RestartMqtt";
        case RuntimeAction::RestartTime: return "RestartTime";
        case RuntimeAction::RestartWiFi: return "RestartWiFi";
        case RuntimeAction::RestartSensorManager: return "RestartSensorManager";
        case RuntimeAction::RestartActuatorRuntime: return "RestartActuatorRuntime";
        case RuntimeAction::RestartDevice: return "RestartDevice";
        default: return "Unknown";
    }
}

RuntimeManager::RuntimeManager(
    ILogger& logger,
    ISensorRuntime* sensorRuntime,
    ActuatorRuntime* actuatorRuntime)
    : logger_(logger)
    , sensorRuntime_(sensorRuntime)
    , actuatorRuntime_(actuatorRuntime) {
}

void RuntimeManager::request(RuntimeAction action) {
    if (action == RuntimeAction::None) return;

    logger_.printf("INFO Runtime action requested: %s\n", runtimeActionName(action));
    if (static_cast<uint8_t>(action) <= static_cast<uint8_t>(pendingAction_)) return;

    if (pendingAction_ != RuntimeAction::None) {
        logger_.printf(
            "INFO Runtime action upgraded: %s -> %s\n",
            runtimeActionName(pendingAction_),
            runtimeActionName(action));
    }
    pendingAction_ = action;
    if (pendingAction_ == RuntimeAction::RestartDevice) {
        logger_.println("INFO Pending restart: device restart required");
    }
}

RuntimeAction RuntimeManager::pendingAction() const {
    return pendingAction_;
}

bool RuntimeManager::restartRequired() const {
    return pendingAction_ != RuntimeAction::None;
}

void RuntimeManager::service() {
    // Subsystem actions intentionally remain pending until restart coordination
    // is implemented by a future milestone.
}

void RuntimeManager::clearPending() {
    pendingAction_ = RuntimeAction::None;
}

bool RuntimeManager::applyPendingSensorChanges() {
    if (pendingAction_ != RuntimeAction::RestartSensorManager || sensorRuntime_ == nullptr) {
        return false;
    }
    logger_.println("Sensor runtime rebuild started");
    size_t activeSensorCount = 0;
    const char* failureReason = nullptr;
    if (!sensorRuntime_->rebuild(activeSensorCount, failureReason)) {
        logger_.printf("Sensor runtime rebuild failed: %s\n",
            failureReason == nullptr ? "unknown failure" : failureReason);
        return false;
    }
    pendingAction_ = RuntimeAction::None;
    logger_.printf("Sensor runtime rebuild successful: %u sensors active\n",
        static_cast<unsigned int>(activeSensorCount));
    return true;
}

bool RuntimeManager::applyPendingActuatorChanges(
    const ActuatorSlotConfiguration* slots) {
    if (pendingAction_ != RuntimeAction::RestartActuatorRuntime
        || actuatorRuntime_ == nullptr
        || slots == nullptr) {
        return false;
    }
    logger_.println("Actuator runtime rebuild started");
    if (!actuatorRuntime_->rebuild(slots)) {
        logger_.println("Actuator runtime rebuild failed");
        return false;
    }
    pendingAction_ = RuntimeAction::None;
    return true;
}

void RuntimeManager::performPendingRestart() {
    if (pendingAction_ != RuntimeAction::RestartDevice) return;
    logger_.println("INFO Device restart executed");
    ESP.restart();
}

} // namespace EnvNode
