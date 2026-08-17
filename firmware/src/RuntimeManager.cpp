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
        case RuntimeAction::RestartControllerRuntime: return "RestartControllerRuntime";
        case RuntimeAction::RestartDevice: return "RestartDevice";
        default: return "Unknown";
    }
}

RuntimeManager::RuntimeManager(
    ILogger& logger,
    ISensorRuntime* sensorRuntime,
    ActuatorRuntime* actuatorRuntime,
    ControllerRuntime* controllerRuntime)
    : logger_(logger)
    , sensorRuntime_(sensorRuntime)
    , actuatorRuntime_(actuatorRuntime)
    , controllerRuntime_(controllerRuntime) {
}

void RuntimeManager::request(RuntimeAction action) {
    if (action == RuntimeAction::None) return;

    logger_.infof("Runtime action requested: %s", runtimeActionName(action));
    if (static_cast<uint8_t>(action) <= static_cast<uint8_t>(pendingAction_)) return;

    if (pendingAction_ != RuntimeAction::None) {
        logger_.infof(
            "Runtime action upgraded: %s -> %s",
            runtimeActionName(pendingAction_),
            runtimeActionName(action));
    }
    pendingAction_ = action;
    if (pendingAction_ == RuntimeAction::RestartDevice) {
        logger_.info("Pending restart: device restart required");
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
    logger_.info("Sensor runtime rebuild started");
    size_t activeSensorCount = 0;
    const char* failureReason = nullptr;
    if (!sensorRuntime_->rebuild(activeSensorCount, failureReason)) {
        logger_.errorf("Sensor runtime rebuild failed: %s",
            failureReason == nullptr ? "unknown failure" : failureReason);
        return false;
    }
    pendingAction_ = RuntimeAction::None;
    logger_.infof("Sensor runtime rebuild successful: %u sensors active",
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
    logger_.info("Actuator runtime rebuild started");
    if (!actuatorRuntime_->rebuild(slots)) {
        logger_.error("Actuator runtime rebuild failed");
        return false;
    }
    pendingAction_ = RuntimeAction::None;
    return true;
}

bool RuntimeManager::applyPendingControllerChanges(
    const ControllerSlotConfiguration* slots) {
    if (pendingAction_ != RuntimeAction::RestartControllerRuntime
        || controllerRuntime_ == nullptr
        || slots == nullptr) {
        return false;
    }
    logger_.info("Controller runtime rebuild started");
    if (!controllerRuntime_->rebuild(slots)) {
        logger_.error("Controller runtime rebuild failed");
        return false;
    }
    pendingAction_ = RuntimeAction::None;
    return true;
}

void RuntimeManager::performPendingRestart() {
    if (pendingAction_ != RuntimeAction::RestartDevice) return;
    logger_.info("Device restart executed");
#if defined(ARDUINO_ARCH_ESP32)
    ESP.restart();
#endif
}

} // namespace EnvNode
