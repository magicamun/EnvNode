#include "IdentityRecordCrc.h"

namespace EnvNode {

uint32_t calculateIdentityRecordCrc32(const uint8_t* data, size_t size) {
    uint32_t crc = 0xFFFFFFFFU;
    for (size_t index = 0; index < size; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            const uint32_t reflectedPolynomial = 0xEDB88320U;
            crc = (crc >> 1) ^ ((crc & 1U) ? reflectedPolynomial : 0U);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

} // namespace EnvNode
