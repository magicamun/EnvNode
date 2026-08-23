#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

class IBoardIdentityStorage {
public:
    virtual ~IBoardIdentityStorage() = default;
    virtual bool read(uint8_t address, uint8_t* data, size_t size) = 0;
    virtual bool write(uint8_t address, const uint8_t* data, size_t size) = 0;
};

} // namespace EnvNode
