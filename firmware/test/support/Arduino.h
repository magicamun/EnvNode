#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef PSTR
#define PSTR(value) (value)
#endif
inline size_t strlen_P(const char* value) { return std::strlen(value); }

class String {
public:
    String() = default;
    String(const char* value)
        : value_(value == nullptr ? "" : value) {
    }
    String(char value) : value_(1, value) {}
    String(int value) : value_(std::to_string(value)) {}
    String(long value) : value_(std::to_string(value)) {}
    String(unsigned long value) : value_(std::to_string(value)) {}
    String(unsigned int value)
        : value_(std::to_string(value)) {
    }

    String(float value, unsigned int precision) {
        char buffer[80];
        std::snprintf(buffer, sizeof(buffer), "%.*f", static_cast<int>(precision), static_cast<double>(value));
        value_ = buffer;
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

    long toInt() const { return std::strtol(value_.c_str(), nullptr, 10); }
    bool endsWith(const char* suffix) const {
        const size_t length = std::strlen(suffix);
        return value_.size() >= length && value_.compare(value_.size()-length, length, suffix) == 0;
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

    bool reserve(size_t size) {
        value_.reserve(size);
        return true;
    }

    String& operator+=(const String& value) {
        value_ += value.value_;
        return *this;
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

class EspClass {
public:
    uint32_t getFreeHeap() const { return 200000; }
    uint32_t getMaxAllocHeap() const { return 100000; }
    uint64_t getEfuseMac() const { return 0x001122334455ULL; }
};

static EspClass ESP;

// Controllable host clock; defaults preserve the previous zero-time stub.
inline uint32_t& arduinoTestMillis() { static uint32_t value = 0; return value; }
inline uint32_t millis() { return arduinoTestMillis(); }

inline void delay(unsigned long duration) { arduinoTestMillis() += duration; }
