#include "PropertySourceInput.h"
#include <cstring>

namespace EnvNode {
bool parsePropertySource(const char* text, PropertySourceInput& result) {
    result = PropertySourceInput{};
    if (text == nullptr) return false;
    size_t length = 0;
    while (length <= MaxPropertySourceLength && text[length]) ++length;
    if (length > MaxPropertySourceLength) return false;
    const char* slash = strchr(text, '/');
    if (slash == nullptr) return false;
    PropertySourceInput parsed;
    const size_t categoryLength = slash - text;
    if (categoryLength == 6 && strncmp(text, "sensor", 6) == 0) parsed.kind = PropertyComponentKind::Sensor;
    else if (categoryLength == 8 && strncmp(text, "actuator", 8) == 0) parsed.kind = PropertyComponentKind::Actuator;
    else if (categoryLength == 10 && strncmp(text, "controller", 10) == 0) parsed.kind = PropertyComponentKind::Controller;
    else if (categoryLength == 6 && strncmp(text, "system", 6) == 0) parsed.kind = PropertyComponentKind::System;
    else if (categoryLength == 5 && strncmp(text, "value", 5) == 0) parsed.kind = PropertyComponentKind::Value;
    else return false;
    const char* current = slash + 1;
    if (*current < '0' || *current > '9') return false;
    uint32_t id = 0;
    while (*current >= '0' && *current <= '9') {
        id = id * 10 + *current++ - '0';
        if (id > UINT16_MAX) return false;
    }
    if (id == 0 || *current++ != '/') return false;
    const size_t keyLength = strlen(current);
    if (keyLength == 0 || keyLength >= sizeof(parsed.key)) return false;
    for (size_t i = 0; i < keyLength; ++i) {
        const char c = current[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
            || (c >= '0' && c <= '9') || c == '_')) return false;
    }
    parsed.id = static_cast<uint16_t>(id);
    memcpy(parsed.key, current, keyLength + 1);
    result = parsed;
    return true;
}

}
