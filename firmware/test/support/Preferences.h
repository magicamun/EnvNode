#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "Arduino.h"

class Preferences {
public:
    bool begin(const char*, bool = false) { return true; }

    bool isKey(const char* key) const {
        return values().count(key == nullptr ? "" : key) != 0;
    }

    String getString(const char* key, const String& fallback = String()) const {
        const auto found = values().find(key == nullptr ? "" : key);
        return found == values().end() ? fallback : String(found->second.c_str());
    }

    uint32_t getUInt(const char* key, uint32_t fallback = 0) const {
        const auto found = values().find(key == nullptr ? "" : key);
        return found == values().end()
            ? fallback : static_cast<uint32_t>(std::stoul(found->second));
    }

    float getFloat(const char* key, float fallback = 0.0F) const {
        const auto found = values().find(key == nullptr ? "" : key);
        return found == values().end() ? fallback : std::stof(found->second);
    }

    static bool& failStringWrites() { static bool fail = false; return fail; }

    size_t putString(const char* key, const String& value) {
        if (failStringWrites()) return 0;
        values()[key] = value.c_str();
        return value.length() + 1;
    }

    size_t putUInt(const char* key, uint32_t value) {
        values()[key] = std::to_string(value);
        return sizeof(value);
    }

    size_t putFloat(const char* key, float value) {
        values()[key] = std::to_string(value);
        return sizeof(value);
    }

    bool remove(const char* key) { return values().erase(key) != 0; }
    bool clear() { values().clear(); return true; }

    static String storedString(const char* key) {
        const auto found = values().find(key);
        return found == values().end() ? String() : String(found->second.c_str());
    }

private:
    static std::map<std::string, std::string>& values() {
        static std::map<std::string, std::string> stored;
        return stored;
    }
};
