#include "SerialLogger.h"
#include <Arduino.h>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>

#ifndef ENVNODE_SERIAL_WRITE_DIAGNOSTICS
#define ENVNODE_SERIAL_WRITE_DIAGNOSTICS 0
#endif

namespace {

uint32_t byteHash(const uint8_t* data, size_t length) {
    uint32_t hash = 2166136261UL;
    for (size_t index = 0; index < length; ++index) {
        hash ^= data[index];
        hash *= 16777619UL;
    }
    return hash;
}

} // namespace

void SerialLogger::begin(unsigned long baud) {
    Serial.begin(baud);
}

void SerialLogger::println(const char* message) {
    if (message != nullptr) {
        writeBytes(
            reinterpret_cast<const uint8_t*>(message),
            strlen(message),
            WriteOperation::PrintlnText);
    }
    static const uint8_t newline[] = {'\r', '\n'};
    writeBytes(newline, sizeof(newline), WriteOperation::PrintlnNewline);
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
        writeBytes(
            reinterpret_cast<const uint8_t*>(output),
            static_cast<size_t>(formattedLength),
            WriteOperation::Formatted);
    }

    if (output != localBuffer) free(output);
}

void SerialLogger::writeBytes(
    const uint8_t* data,
    size_t length,
    WriteOperation operation) {
    if (data == nullptr || length == 0) return;
    ++writeSequence_;
#if ENVNODE_SERIAL_WRITE_DIAGNOSTICS
    const char* operationText = "unknown";
    switch (operation) {
        case WriteOperation::PrintlnText: operationText = "println"; break;
        case WriteOperation::PrintlnNewline: operationText = "newline"; break;
        case WriteOperation::Formatted: operationText = "printf"; break;
    }
    char diagnostic[112];
    const int diagnosticLength = snprintf(
        diagnostic,
        sizeof(diagnostic),
        "[SL seq=%lu op=%s len=%u hash=%08lX first=%02X%02X%02X%02X]\r\n",
        static_cast<unsigned long>(writeSequence_),
        operationText,
        static_cast<unsigned int>(length),
        static_cast<unsigned long>(byteHash(data, length)),
        length > 0 ? data[0] : 0,
        length > 1 ? data[1] : 0,
        length > 2 ? data[2] : 0,
        length > 3 ? data[3] : 0);
    if (diagnosticLength > 0
        && static_cast<size_t>(diagnosticLength) < sizeof(diagnostic)) {
        Serial.write(
            reinterpret_cast<const uint8_t*>(diagnostic),
            static_cast<size_t>(diagnosticLength));
    }
#else
    (void)operation;
#endif
    Serial.write(data, length);
}
