#include "JsonWriter.h"

#include <cstdint>

namespace EnvNode {

void appendJsonString(String& output, const char* value) {
    output += '"';
    if (value != nullptr) {
        for (const char* cursor = value; *cursor != '\0'; ++cursor) {
            switch (*cursor) {
                case '"': output += "\\\""; break;
                case '\\': output += "\\\\"; break;
                case '\b': output += "\\b"; break;
                case '\f': output += "\\f"; break;
                case '\n': output += "\\n"; break;
                case '\r': output += "\\r"; break;
                case '\t': output += "\\t"; break;
                default: {
                    const uint8_t byte = static_cast<uint8_t>(*cursor);
                    if (byte >= 0x20) {
                        output += *cursor;
                    } else {
                        const char hex[] = "0123456789abcdef";
                        output += "\\u00";
                        output += hex[(byte >> 4) & 0x0F];
                        output += hex[byte & 0x0F];
                    }
                    break;
                }
            }
        }
    }
    output += '"';
}

} // namespace EnvNode
