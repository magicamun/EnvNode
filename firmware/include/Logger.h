#pragma once

class ILogger {
public:
    virtual ~ILogger() = default;

    virtual void begin(unsigned long baud) = 0;
    virtual void println(const char* message) = 0;
    virtual void printf(const char* format, ...) = 0;
};
