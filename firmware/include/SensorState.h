#pragma once

namespace WeatherStation {

enum class SensorState {
    Unknown,
    Initializing,
    Ready,
    Degraded,
    Failed,
    Simulated,
};

} // namespace WeatherStation
