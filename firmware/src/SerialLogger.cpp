#include "SerialLogger.h"
#include <Arduino.h>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>

void SerialLogger::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialLogger::println(const char* message) {
    Serial.println(message);
}

void SerialLogger::printf(const char* format, ...) {
    if (format == nullptr) return;

    char localBuffer[128];
    va_list args;
    va_start(args, format);

    va_list lengthArgs;
    va_copy(lengthArgs, args);
    const int requiredLength = vsnprintf(nullptr, 0, format, lengthArgs);
    va_end(lengthArgs);
    if (requiredLength < 0) {
        va_end(args);
        return;
    }

    char* output = localBuffer;
    if (static_cast<size_t>(requiredLength) >= sizeof(localBuffer)) {
        output = static_cast<char*>(malloc(static_cast<size_t>(requiredLength) + 1));
        if (output == nullptr) {
            va_end(args);
            return;
        }
    }

    const size_t outputCapacity = static_cast<size_t>(requiredLength) + 1;
    const int formattedLength = vsnprintf(output, outputCapacity, format, args);
    va_end(args);
    if (formattedLength >= 0) {
        Serial.write(
            reinterpret_cast<const uint8_t*>(output),
            static_cast<size_t>(formattedLength));
    }

    if (output != localBuffer) free(output);
}
