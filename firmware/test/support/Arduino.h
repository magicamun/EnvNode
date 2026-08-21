#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

class String {
public:
    String() = default;
    String(const char* value)
        : value_(value == nullptr ? "" : value) {
    }
    String(unsigned int value)
        : value_(std::to_string(value)) {
    }

    String& operator=(const char* value) {
        value_ = value == nullptr ? "" : value;
        return *this;
    }

    const char* c_str() const {
        return value_.c_str();
    }

    bool isEmpty() const {
        return value_.empty();
    }

    size_t length() const {
        return value_.length();
    }

    char charAt(size_t index) const {
        return index < value_.length() ? value_[index] : '\0';
    }

    char operator[](size_t index) const {
        return value_[index];
    }

    void reserve(size_t size) {
        value_.reserve(size);
    }

    String& operator+=(const char* value) {
        value_ += value == nullptr ? "" : value;
        return *this;
    }

    String& operator+=(char value) {
        value_ += value;
        return *this;
    }

    friend String operator+(const String& left, const char* right) {
        String result(left);
        result += right;
        return result;
    }

    friend String operator+(const String& left, const String& right) {
        String result(left);
        result.value_ += right.value_;
        return result;
    }


    friend String operator+(const char* left, const String& right) {
        String result(left);
        result.value_ += right.value_;
        return result;
    }

    friend bool operator==(const String& left, const String& right) {
        return left.value_ == right.value_;
    }

    friend bool operator!=(const String& left, const String& right) {
        return !(left == right);
    }

private:
    std::string value_;
};

constexpr int OUTPUT = 1;
constexpr int INPUT = 0;
constexpr int LOW = 0;
constexpr int HIGH = 1;

void pinMode(unsigned char pin, int mode);
void digitalWrite(unsigned char pin, int value);

inline uint32_t ledcSetup(uint8_t, uint32_t frequency, uint8_t) { return frequency; }
inline void ledcWrite(uint8_t, uint32_t) {}
inline void ledcAttachPin(uint8_t, uint8_t) {}
inline void ledcDetachPin(uint8_t) {}

class HardwareSerial {
public:
    void begin(unsigned long baud);
    void println(const char* value);
    size_t write(const uint8_t* data, size_t length);
};

extern HardwareSerial Serial;
