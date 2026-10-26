#pragma once

#include "LogEntry.h"

namespace EnvNode {

class ILogEntrySink {
public:
    virtual ~ILogEntrySink() = default;

    virtual void begin(unsigned long baud) = 0;
    virtual void write(const LogEntry& entry) = 0;
};

} // namespace EnvNode
