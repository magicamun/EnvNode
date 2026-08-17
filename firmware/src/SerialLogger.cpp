#include "SerialLogger.h"
#include <Arduino.h>
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

void SerialLogger::write(const EnvNode::LogEntry& entry) {
    char line[EnvNode::LogMessageCapacity + 2];
    const size_t messageLength = strnlen(
        entry.message, EnvNode::LogMessageCapacity - 1);
    memcpy(line, entry.message, messageLength);
    line[messageLength] = '\r';
    line[messageLength + 1] = '\n';
    writeBytes(
        reinterpret_cast<const uint8_t*>(line),
        messageLength + 2,
        WriteOperation::CanonicalEntry);
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
        case WriteOperation::CanonicalEntry: operationText = "entry"; break;
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
