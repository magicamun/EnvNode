#pragma once

#include <cstddef>
#include <cstdint>

namespace EnvNode {

class IdentityRecordCrc32 {
public:
    void update(const uint8_t* data, size_t size);
    uint32_t value() const;

private:
    uint32_t crc_ = 0xFFFFFFFFU;
};

uint32_t calculateIdentityRecordCrc32(const uint8_t* data, size_t size);

} // namespace EnvNode
