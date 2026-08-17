#pragma once

#include <cstddef>
#include <string>

class String {
public:
    String() = default;
    String(const char* value)
        : value_(value == nullptr ? "" : value) {
    }

    String& operator=(const char* value) {
        value_ = value == nullptr ? "" : value;
        return *this;
    }

    const char* c_str() const {
        return value_.c_str();
    }

private:
    std::string value_;
};

constexpr int OUTPUT = 1;
constexpr int LOW = 0;
constexpr int HIGH = 1;

void pinMode(unsigned char pin, int mode);
void digitalWrite(unsigned char pin, int value);
