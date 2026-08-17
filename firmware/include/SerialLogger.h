#pragma once

#include <cstddef>
#include <cstdint>

#include "Logger.h"

class SerialLogger : public ILogger {
public:
    void begin(unsigned long baud) override;
    void println(const char* message) override;
    void printf(const char* format, ...) override;

private:
    enum class WriteOperation : uint8_t {
        PrintlnText,
        PrintlnNewline,
        Formatted,
    };

    void writeBytes(
        const uint8_t* data,
        size_t length,
        WriteOperation operation);

    uint32_t writeSequence_ = 0;
};
