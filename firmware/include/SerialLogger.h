#pragma once

#include <cstddef>
#include <cstdint>

#include "ILogEntrySink.h"

class SerialLogger : public EnvNode::ILogEntrySink {
public:
    void begin(unsigned long baud) override;
    void write(const EnvNode::LogEntry& entry) override;

private:
    enum class WriteOperation : uint8_t {
        CanonicalEntry,
    };

    void writeBytes(
        const uint8_t* data,
        size_t length,
        WriteOperation operation);

    uint32_t writeSequence_ = 0;
};
