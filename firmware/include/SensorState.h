#pragma once

namespace WeatherStation {

enum class SensorState {
    Unknown,
    Initializing,
    Ready,
    Degraded,
    Failed,
};

inline bool isSensorAvailable(SensorState state) {
    return state == SensorState::Ready || state == SensorState::Degraded;
}

} // namespace WeatherStation
