#pragma once

#include <Arduino.h>

#include "Logger.h"
#include "RuntimeManager.h"

namespace EnvNode {

enum class OTAState : uint8_t {
    Idle,
    Uploading,
    Verifying,
    FirmwareStaged,
    Failed,
};

const char* otaStateName(OTAState state);

class OTAService {
public:
    OTAService(ILogger& logger, RuntimeManager& runtimeManager);

    bool beginUpload(size_t totalBytes = 0);
    bool writeChunk(uint8_t* data, size_t length);
    bool finishUpload();
    void abortUpload(const char* reason);
    void rejectUpload(const char* reason);

    OTAState state() const;
    size_t bytesReceived() const;
    size_t totalBytes() const;
    bool totalBytesKnown() const;
    uint8_t progressPercent() const;
    const String& lastError() const;
    bool firmwareStaged() const;
    bool restartRequired() const;
    bool busy() const;

private:
    void fail(const String& reason, bool abortUpdate);
    String updateError(const char* fallback) const;

    ILogger& logger_;
    RuntimeManager& runtimeManager_;
    OTAState state_ = OTAState::Idle;
    size_t bytesReceived_ = 0;
    size_t totalBytes_ = 0;
    String lastError_;
};

} // namespace EnvNode
