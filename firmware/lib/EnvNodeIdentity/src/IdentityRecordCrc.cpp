#include "IdentityRecordCrc.h"

namespace EnvNode {

void IdentityRecordCrc32::update(const uint8_t* data, size_t size) {
    for (size_t index = 0; index < size; ++index) {
        crc_ ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint32_t reflectedPolynomial = 0xEDB88320U;
            crc_ = (crc_ >> 1) ^ ((crc_ & 1U) ? reflectedPolynomial : 0U);
        }
    }
}

uint32_t IdentityRecordCrc32::value() const {
    return crc_ ^ 0xFFFFFFFFU;
}

uint32_t calculateIdentityRecordCrc32(const uint8_t* data, size_t size) {
    IdentityRecordCrc32 crc;
    crc.update(data, size);
    return crc.value();
}

} // namespace EnvNode
