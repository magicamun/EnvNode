#include "Logger.h"

#include <cstdarg>
#include <cstdio>

#include "LogEntry.h"

bool ILogger::accepts(EnvNode::LogLevel) const {
    return true;
}

void ILogger::log(EnvNode::LogLevel, const char* message) {
    println(message);
}

void ILogger::debug(const char* message) {
    if (accepts(EnvNode::LogLevel::Debug)) log(EnvNode::LogLevel::Debug, message);
}

void ILogger::info(const char* message) {
    if (accepts(EnvNode::LogLevel::Info)) log(EnvNode::LogLevel::Info, message);
}

void ILogger::warn(const char* message) {
    if (accepts(EnvNode::LogLevel::Warn)) log(EnvNode::LogLevel::Warn, message);
}

void ILogger::error(const char* message) {
    if (accepts(EnvNode::LogLevel::Error)) log(EnvNode::LogLevel::Error, message);
}

void ILogger::logFormatted(
    EnvNode::LogLevel level,
    const char* format,
    va_list args) {
    if (!accepts(level) || format == nullptr) return;
    char message[EnvNode::LogMessageCapacity];
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

void ILogger::debugf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(EnvNode::LogLevel::Debug, format, args);
    va_end(args);
}

void ILogger::infof(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(EnvNode::LogLevel::Info, format, args);
    va_end(args);
}

void ILogger::warnf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(EnvNode::LogLevel::Warn, format, args);
    va_end(args);
}

void ILogger::errorf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    logFormatted(EnvNode::LogLevel::Error, format, args);
    va_end(args);
}
