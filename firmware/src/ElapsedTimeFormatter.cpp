#include "ElapsedTimeFormatter.h"

namespace EnvNode {

String formatElapsedDuration(uint32_t elapsedMs) {
    static constexpr uint32_t MillisecondsPerSecond = 1000UL;
    static constexpr uint32_t MillisecondsPerMinute = 60UL * MillisecondsPerSecond;
    static constexpr uint32_t MillisecondsPerHour = 60UL * MillisecondsPerMinute;
    static constexpr uint32_t MillisecondsPerDay = 24UL * MillisecondsPerHour;

    if (elapsedMs < MillisecondsPerMinute) {
        return String(elapsedMs / MillisecondsPerSecond) + " s";
    }
    if (elapsedMs < MillisecondsPerHour) {
        return String(elapsedMs / MillisecondsPerMinute) + " min";
    }
    if (elapsedMs < MillisecondsPerDay) {
        return String(elapsedMs / MillisecondsPerHour) + " h";
    }
    return String(elapsedMs / MillisecondsPerDay) + " d";
}

} // namespace EnvNode
