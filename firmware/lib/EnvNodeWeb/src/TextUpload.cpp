// EnvNode: see repository LICENSE.
#include "TextUpload.h"
#include <cstring>
namespace EnvNode {
TextUploadResult parseTextUploadSize(const String& hint, size_t maximum, size_t& size) {
    size = 0;
    if (hint.isEmpty()) return TextUploadResult::Success;
    for (size_t index = 0; index < hint.length(); ++index) {
        const char digit = hint[index];
        if (digit < '0' || digit > '9') return TextUploadResult::InvalidSize;
        const size_t value = static_cast<size_t>(digit - '0');
        if (size > maximum / 10 || (size == maximum / 10 && value > maximum % 10)) return TextUploadResult::TooLarge;
        size = size * 10 + value;
    }
    return size == 0 ? TextUploadResult::InvalidSize : TextUploadResult::Success;
}
TextUploadResult prepareTextUpload(String& text, const String& hint, size_t maximum) {
    size_t expected = 0;
    const TextUploadResult parsed = parseTextUploadSize(hint, maximum, expected);
    if (parsed != TextUploadResult::Success) return parsed;
    if (expected && !text.reserve(expected)) return TextUploadResult::OutOfMemory;
    return TextUploadResult::Success;
}
TextUploadResult appendTextUpload(String& text, const uint8_t* data, size_t size, size_t maximum) {
    if (text.length() > maximum || size > maximum - text.length()) return TextUploadResult::TooLarge;
    if (size && !data) return TextUploadResult::InvalidSize;
    // Some Arduino String implementations read length+1 bytes in concat().
    // Bound the copy and supply our own terminator, including full upload chunks.
    char chunk[257];
    while (size) {
        const size_t count = size < sizeof(chunk)-1 ? size : sizeof(chunk)-1;
        memcpy(chunk, data, count); chunk[count] = '\0';
        if (!text.concat(chunk, count)) return TextUploadResult::OutOfMemory;
        data += count; size -= count;
    }
    return TextUploadResult::Success;
}
const char* textUploadResultName(TextUploadResult result) {
    switch (result) {
        case TextUploadResult::Success: return "ok";
        case TextUploadResult::InvalidSize: return "invalid-size";
        case TextUploadResult::TooLarge: return "too-large";
        case TextUploadResult::OutOfMemory: return "out-of-memory";
    }
    return "unknown";
}
}
