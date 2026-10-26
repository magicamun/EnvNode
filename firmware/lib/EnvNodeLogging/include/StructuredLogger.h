#pragma once

#include <cstdarg>

#include "ILogEntrySink.h"
#include "ILogTimeProvider.h"
#include "Logger.h"
#include "RecentLogStore.h"

namespace EnvNode {

class StructuredLogger : public ILogger {
public:
    StructuredLogger(
        RecentLogStore& store,
        ILogTimeProvider& timeProvider,
        ILogEntrySink& sink,
        LogLevel minimumLevel = LogLevel::Info);

    void begin(unsigned long baud) override;
    void println(const char* message) override;
    void printf(const char* format, ...) override
        __attribute__((format(printf, 2, 3)));
    bool accepts(LogLevel level) const override;
    void log(LogLevel level, const char* message) override;

    LogLevel minimumLevel() const;
    void setMinimumLevel(LogLevel level);

private:
    void logFormatted(LogLevel level, const char* format, va_list args);
    void populateMessage(LogEntry& entry, const char* message) const;

    RecentLogStore& store_;
    ILogTimeProvider& timeProvider_;
    ILogEntrySink& sink_;
    LogLevel minimumLevel_;
    uint32_t nextSequence_ = 1;
};

} // namespace EnvNode
