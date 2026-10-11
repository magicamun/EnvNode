#pragma once

#include <cstdint>
#include <cstring>
#include <map>
#include <string>

#include "Arduino.h"

class Preferences {
public:
    bool begin(const char* name, bool = false) { namespace_ = name; return true; }
    void end() {}

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
        return value.length();
    }

    size_t getBytesLength(const char* key) const {
        const auto found = values().find(key);
        return found == values().end() ? 0 : found->second.size();
    }
    size_t getBytes(const char* key, void* target, size_t length) const {
        if (failByteReads()) return 0;
        const auto found = values().find(key);
        if (found == values().end() || length < found->second.size()) return 0;
        memcpy(target, found->second.data(), found->second.size());
        return found->second.size();
    }
    size_t putBytes(const char* key, const void* bytes, size_t length) {
        if (failStringWrites()) return 0;
        values()[key] = std::string(static_cast<const char*>(bytes), length);
        return length;
    }
    static String& failUIntKey() { static String key; return key; }
    static bool& failByteReads() { static bool fail = false; return fail; }
    size_t putUInt(const char* key, uint32_t value) {
        if (failUIntKey() == key) return 0;
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
        const auto& stored = namespaces()["weather"];
        const auto found = stored.find(key);
        return found == stored.end() ? String() : String(found->second.c_str());
    }

private:
    std::string namespace_ = "weather";
    static std::map<std::string, std::map<std::string, std::string>>& namespaces() {
        static std::map<std::string, std::map<std::string, std::string>> stored;
        return stored;
    }
    std::map<std::string, std::string>& values() const { return namespaces()[namespace_]; }
};
