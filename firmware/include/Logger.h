#pragma once

#include <cstdarg>

#include "LogLevel.h"

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void begin(unsigned long baud) = 0;
    virtual void println(const char* message) = 0;
    virtual void printf(const char* format, ...)
        __attribute__((format(printf, 2, 3))) = 0;

    virtual bool accepts(EnvNode::LogLevel level) const;
    virtual void log(EnvNode::LogLevel level, const char* message);

    void debug(const char* message);
    void info(const char* message);
    void warn(const char* message);
    void error(const char* message);
    void debugf(const char* format, ...)
        __attribute__((format(printf, 2, 3)));
    void infof(const char* format, ...)
        __attribute__((format(printf, 2, 3)));
    void warnf(const char* format, ...)
        __attribute__((format(printf, 2, 3)));
    void errorf(const char* format, ...)
        __attribute__((format(printf, 2, 3)));

private:
    void logFormatted(
        EnvNode::LogLevel level,
        const char* format,
        va_list args);
};
