// EnvNode: see repository LICENSE.
#pragma once
#include <cstdint>
#include <cstddef>
#include "Logger.h"
namespace EnvNode {
struct SdCardDiagnosticInfo {
    const char* type = "unknown";
    uint64_t cardBytes = 0;
    uint64_t filesystemBytes = 0;
    uint64_t usedBytes = 0;
    bool rootReadable = false;
};
enum class SdTestFileCreate { Created, Exists, Failed };
class ISdTestFile {
public:
    virtual ~ISdTestFile() = default;
    virtual SdTestFileCreate createExclusive(const char* path) = 0;
    virtual int write(const uint8_t* data, size_t size) = 0;
    virtual bool sync() = 0;
    virtual bool close() = 0;
    virtual bool openRead(const char* path) = 0;
    virtual int read(uint8_t* data, size_t size) = 0;
    virtual bool remove(const char* path) = 0;
};
bool runSdWriteReadTest(ISdTestFile& file, ILogger& logger);
class ISdCardDiagnosticDevice {
public:
    virtual ~ISdCardDiagnosticDevice() = default;
    virtual bool cardDetected() = 0;
    virtual bool mount() = 0;
    virtual SdCardDiagnosticInfo information() = 0;
    virtual void release() = 0;
    virtual bool writeReadTest(ILogger& logger) = 0;
};
enum class SdCardDiagnosticResult { NoCard, MountFailed, ReadFailed, WriteReadFailed, Passed };
// One-shot startup test. Device adapter owns SPI and filesystem resources.
SdCardDiagnosticResult runSdCardDiagnostic(ISdCardDiagnosticDevice& device, ILogger& logger);
}
