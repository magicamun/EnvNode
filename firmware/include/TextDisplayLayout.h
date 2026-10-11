// SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
// See LICENSE. Shared geometry and character clipping for display output and preview.
#pragma once
#include "TextDisplayConfiguration.h"
#include <cstring>
namespace EnvNode {
struct TextDisplayLayout {
    uint16_t pixelWidth = 0, pixelHeight = 0;
    uint8_t cellWidth = 0, cellHeight = 0, baseline = 0;
    size_t columns() const { return cellWidth ? pixelWidth / cellWidth : 0; }
    size_t rows() const { return cellHeight ? pixelHeight / cellHeight : 0; }
};
inline TextDisplayLayout textDisplayLayout(const TextDisplayConfiguration& config) {
    TextDisplayLayout result;
    if (!textDisplayTypeName(config.type)) return result;
    result.pixelWidth = 128; result.pixelHeight = 64;
    switch (config.font) {
    case TextDisplayFont::Fixed6x10: result.cellWidth = 6; result.cellHeight = 10; result.baseline = 8; break;
    case TextDisplayFont::Fixed8x13: result.cellWidth = 8; result.cellHeight = 13; result.baseline = 11; break;
    }
    return result;
}
// The selected fonts contain printable ASCII and Latin-1. Unsupported codepoints
// occupy one cell as '?'. Never split a UTF-8 character or a display cell.
inline void clipDisplayText(const char* input, size_t columns, char* output, size_t capacity) {
    if (!capacity) return;
    size_t written = 0, cells = 0;
    const size_t length = strlen(input);
    for (size_t i = 0; i < length && cells < columns; ++cells) {
        const unsigned char first = static_cast<unsigned char>(input[i]);
        size_t bytes = first < 0x80 ? 1 : first >= 0xC2 && first <= 0xDF ? 2
            : first >= 0xE0 && first <= 0xEF ? 3 : first >= 0xF0 && first <= 0xF4 ? 4 : 1;
        uint32_t code = bytes == 1 ? first : first & ((1U << (7 - bytes)) - 1);
        bool valid = first < 0x80 || bytes > 1;
        if (i + bytes > length) valid = false;
        if (valid) for (size_t k = 1; k < bytes; ++k) {
            const unsigned char next = static_cast<unsigned char>(input[i + k]);
            if ((next & 0xC0) != 0x80) { valid = false; break; }
            code = (code << 6) | (next & 0x3F);
        }
        if (!valid) bytes = 1;
        const bool supported = valid && ((bytes == 1 && code >= 32 && code <= 126)
            || (bytes == 2 && code >= 160 && code <= 255));
        const size_t count = supported ? bytes : 1;
        if (written + count >= capacity) break;
        if (supported) memmove(output + written, input + i, count);
        else output[written] = '?';
        written += count; i += bytes;
    }
    output[written] = 0;
}
}
