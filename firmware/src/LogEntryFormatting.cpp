#include "LogEntryFormatting.h"

#include <cstdio>
#include <ctime>

namespace EnvNode {

bool formatLogEntryTimestamp(
    const LogEntry& entry,
    char* output,
    size_t outputSize) {
    if (output == nullptr || outputSize == 0) return false;
    if (entry.wallClockValid) {
        const time_t epoch = static_cast<time_t>(entry.epochSeconds);
        tm localTime;
        if (localtime_r(&epoch, &localTime) == nullptr) return false;
        return strftime(output, outputSize, "%Y-%m-%d %H:%M:%S", &localTime) > 0;
    }
    const uint32_t totalSeconds = entry.monotonicMs / 1000UL;
    const uint32_t hours = totalSeconds / 3600UL;
    const uint32_t minutes = (totalSeconds / 60UL) % 60UL;
    const uint32_t seconds = totalSeconds % 60UL;
    const uint32_t milliseconds = entry.monotonicMs % 1000UL;
    const int length = snprintf(
        output,
        outputSize,
        "+%02lu:%02lu:%02lu.%03lu",
        static_cast<unsigned long>(hours),
        static_cast<unsigned long>(minutes),
        static_cast<unsigned long>(seconds),
        static_cast<unsigned long>(milliseconds));
    return length > 0 && static_cast<size_t>(length) < outputSize;
}

} // namespace EnvNode
