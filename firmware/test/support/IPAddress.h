#pragma once

#include <cstdint>
#include <cstdio>

#include "Arduino.h"

class IPAddress {
public:
    bool fromString(const String& value) {
        unsigned int parsed[4];
        char trailing = '\0';
        if (std::sscanf(value.c_str(), "%u.%u.%u.%u%c",
                &parsed[0], &parsed[1], &parsed[2], &parsed[3], &trailing) != 4) {
            return false;
        }
        for (size_t index = 0; index < 4; ++index) {
            if (parsed[index] > 255) return false;
            bytes_[index] = static_cast<uint8_t>(parsed[index]);
        }
        return true;
    }

    uint8_t operator[](size_t index) const { return bytes_[index]; }

    explicit operator uint32_t() const {
        return static_cast<uint32_t>(bytes_[0])
            | static_cast<uint32_t>(bytes_[1]) << 8
            | static_cast<uint32_t>(bytes_[2]) << 16
            | static_cast<uint32_t>(bytes_[3]) << 24;
    }

private:
    uint8_t bytes_[4] = {};
};
