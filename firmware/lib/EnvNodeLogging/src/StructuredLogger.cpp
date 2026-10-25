#include "StructuredLogger.h"

#include <cstdio>
#include <cstring>

namespace EnvNode {

StructuredLogger::StructuredLogger(
    RecentLogStore& store,
    ILogTimeProvider& timeProvider,
    ILogEntrySink& sink,
    LogLevel minimumLevel)
    : store_(store)
    , timeProvider_(timeProvider)
    , sink_(sink)
    , minimumLevel_(minimumLevel) {
}

void StructuredLogger::begin(unsigned long baud) {
    sink_.begin(baud);
}

void StructuredLogger::println(const char* message) {
    log(LogLevel::Info, message);
}

void StructuredLogger::printf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(LogLevel::Info, format, args);
    va_end(args);
}

bool StructuredLogger::accepts(LogLevel level) const {
    return static_cast<uint8_t>(level) >= static_cast<uint8_t>(minimumLevel_);
}

void StructuredLogger::log(LogLevel level, const char* message) {
    if (!accepts(level)) return;
    LogEntry entry;
    entry.sequence = nextSequence_++;
    entry.monotonicMs = timeProvider_.monotonicMs();
    entry.wallClockValid =
        timeProvider_.wallClockEpochSeconds(entry.epochSeconds);
    if (!entry.wallClockValid) entry.epochSeconds = 0;
    entry.level = level;
    populateMessage(entry, message);
    store_.append(entry);
    sink_.write(entry);
}

LogLevel StructuredLogger::minimumLevel() const {
    return minimumLevel_;
}

void StructuredLogger::setMinimumLevel(LogLevel level) {
    minimumLevel_ = level;
}

void StructuredLogger::logFormatted(
    LogLevel level,
    const char* format,
    va_list args) {
    if (!accepts(level) || format == nullptr) return;
    char message[LogMessageCapacity];
    const int length = vsnprintf(message, sizeof(message), format, args);
    if (length < 0) return;
    if (static_cast<size_t>(length) >= sizeof(message)) {
        message[sizeof(message) - 4] = '.';
        message[sizeof(message) - 3] = '.';
        message[sizeof(message) - 2] = '.';
        message[sizeof(message) - 1] = '\0';
    }
    log(level, message);
}

void StructuredLogger::populateMessage(
    LogEntry& entry,
    const char* message) const {
    if (message == nullptr) return;
    size_t sourceLength = strlen(message);
    while (sourceLength > 0
        && (message[sourceLength - 1] == '\n'
            || message[sourceLength - 1] == '\r')) {
        --sourceLength;
    }

    size_t outputIndex = 0;
    bool truncated = false;
    for (size_t sourceIndex = 0; sourceIndex < sourceLength; ++sourceIndex) {
        char value = message[sourceIndex];
        if (value == '\r' || value == '\n') {
            if (value == '\r' && sourceIndex + 1 < sourceLength
                && message[sourceIndex + 1] == '\n') ++sourceIndex;
            value = ' ';
        }
        if (outputIndex < LogMessageCapacity - 1) {
            entry.message[outputIndex++] = value;
        } else {
            truncated = true;
        }
    }
    if (truncated) {
        entry.message[LogMessageCapacity - 4] = '.';
        entry.message[LogMessageCapacity - 3] = '.';
        entry.message[LogMessageCapacity - 2] = '.';
        outputIndex = LogMessageCapacity - 1;
    }
    entry.message[outputIndex] = '\0';
}

} // namespace EnvNode
