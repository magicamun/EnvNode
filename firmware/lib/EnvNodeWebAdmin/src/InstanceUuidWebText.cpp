// EnvNode: see repository LICENSE.
#include "InstanceUuidWebText.h"
namespace EnvNode {
String descriptorUuid(const uint8_t (&value)[16]) {
    static const char Hex[] = "0123456789abcdef";
    String result;
    result.reserve(36);
    for (size_t index = 0; index < sizeof(value); ++index) {
        if (index == 4 || index == 6 || index == 8 || index == 10) result += '-';
        result += Hex[value[index] >> 4];
        result += Hex[value[index] & 0x0F];
    }
    return result;
}
}
