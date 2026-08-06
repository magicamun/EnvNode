#include "SerialLogger.h"
#include <Arduino.h>
#include <cstdarg>
#include <cstdio>

void SerialLogger::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialLogger::println(const char* message) {
    Serial.println(message);
}

void SerialLogger::printf(const char* format, ...) {
    char buffer[128];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    Serial.print(buffer);
}
