// EnvNode native update double: see repository LICENSE.
#pragma once
#include "Arduino.h"
#include <algorithm>
#include <limits>
constexpr size_t UPDATE_SIZE_UNKNOWN = 0xffffffff;
constexpr int U_FLASH = 0;
class UpdateClass {
public:
    bool beginOk = true, endOk = true, finalizeError = false;
    bool running = false, error = false, endEvenIfRemaining = false;
    int begins = 0, ends = 0, aborts = 0;
    size_t requestedSize = 0, writeLimit = std::numeric_limits<size_t>::max();
    const char* detail = "driver failure";
    void clearError() { error = false; }
    bool begin(size_t size, int) {
        ++begins; requestedSize = size; running = beginOk; error = !beginOk; return beginOk;
    }
    size_t write(uint8_t*, size_t size) {
        const size_t written = std::min(size, writeLimit); if (written != size) error = true; return written;
    }
    bool end(bool evenIfRemaining) {
        ++ends; endEvenIfRemaining = evenIfRemaining; running = false;
        error = !endOk || finalizeError; return endOk;
    }
    bool hasError() const { return error; }
    const char* errorString() const { return detail; }
    bool isRunning() const { return running; }
    void abort() { ++aborts; running = false; }
};
extern UpdateClass Update;
