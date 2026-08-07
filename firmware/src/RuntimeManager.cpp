#include "RuntimeManager.h"

#include <Arduino.h>

namespace WeatherStation {

const char* runtimeActionName(RuntimeAction action) {
    switch (action) {
        case RuntimeAction::None: return "None";
        case RuntimeAction::RestartMqtt: return "RestartMqtt";
        case RuntimeAction::RestartTime: return "RestartTime";
        case RuntimeAction::RestartWiFi: return "RestartWiFi";
        case RuntimeAction::RestartSensorManager: return "RestartSensorManager";
        case RuntimeAction::RestartDevice: return "RestartDevice";
        default: return "Unknown";
    }
}

RuntimeManager::RuntimeManager(ILogger& logger)
    : logger_(logger) {
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

void RuntimeManager::performPendingRestart() {
    if (pendingAction_ != RuntimeAction::RestartDevice) return;
    logger_.println("INFO Device restart executed");
    ESP.restart();
}

} // namespace WeatherStation
