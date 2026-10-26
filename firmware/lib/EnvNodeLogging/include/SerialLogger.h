#pragma once

#include <cstddef>
#include <cstdint>

#include "ILogEntrySink.h"

class SerialLogger : public EnvNode::ILogEntrySink {
public:
    static constexpr size_t RenderBufferSize = 320;

    void begin(unsigned long baud) override;
    void write(const EnvNode::LogEntry& entry) override;
    uint32_t shortWriteCount() const;
    uint32_t renderFailureCount() const;

private:
    enum class WriteOperation : uint8_t {
        CanonicalEntry,
    };

    void writeBytes(
        const uint8_t* data,
        size_t length,
        WriteOperation operation);
    bool initialized_ = false;
    uint32_t writeSequence_ = 0;
    uint32_t shortWriteCount_ = 0;
    uint32_t renderFailureCount_ = 0;
};
