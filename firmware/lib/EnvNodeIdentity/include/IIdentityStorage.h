#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

class IIdentityStorage {
public:
    virtual ~IIdentityStorage() = default;
    virtual bool read(uint16_t address, uint8_t* data, size_t size) = 0;
    virtual bool write(uint16_t address, const uint8_t* data, size_t size) = 0;
};

} // namespace EnvNode
