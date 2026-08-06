#pragma once

#include "Logger.h"

class SerialLogger : public ILogger {
public:
    void begin(unsigned long baud) override;
    void println(const char* message) override;
    void printf(const char* format, ...) override;
};
