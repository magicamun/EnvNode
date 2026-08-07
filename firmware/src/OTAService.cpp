#include "OTAService.h"

#include <Update.h>

namespace WeatherStation {

const char* otaStateName(OTAState state) {
    switch (state) {
        case OTAState::Idle: return "Idle";
        case OTAState::Uploading: return "Uploading";
        case OTAState::Verifying: return "Verifying";
        case OTAState::FirmwareStaged: return "Firmware staged";
        case OTAState::Failed: return "Failed";
        default: return "Unknown";
    }
}

OTAService::OTAService(ILogger& logger, RuntimeManager& runtimeManager)
    : logger_(logger), runtimeManager_(runtimeManager) {
}

bool OTAService::beginUpload(size_t totalBytes) {
    if (busy() || firmwareStaged()) return false;

    bytesReceived_ = 0;
    totalBytes_ = totalBytes;
    lastError_ = String();
    Update.clearError();
    const size_t updateSize = totalBytes_ == 0 ? UPDATE_SIZE_UNKNOWN : totalBytes_;
    if (!Update.begin(updateSize, U_FLASH)) {
        fail(updateError("Unable to initialize the firmware update."), false);
        return false;
    }

    state_ = OTAState::Uploading;
    logger_.println("INFO OTA upload started");
    if (totalBytesKnown()) {
        logger_.printf("INFO OTA expected size: %u bytes\n", static_cast<unsigned int>(totalBytes_));
    } else {
        logger_.println("INFO OTA expected size: unknown");
    }
    return true;
}

bool OTAService::writeChunk(uint8_t* data, size_t length) {
    if (state_ != OTAState::Uploading || data == nullptr || length == 0) return false;
    const size_t written = Update.write(data, length);
    if (written != length) {
        bytesReceived_ += written;
        fail(updateError("Firmware data could not be written completely."), true);
        return false;
    }
    bytesReceived_ += written;
    return true;
}

bool OTAService::finishUpload() {
    if (state_ != OTAState::Uploading) return false;
    if (bytesReceived_ == 0) {
        fail("The uploaded firmware image is empty.", true);
        return false;
    }
    if (totalBytesKnown() && bytesReceived_ != totalBytes_) {
        fail("The firmware upload ended before the complete image was received.", true);
        return false;
    }

    state_ = OTAState::Verifying;
    // With a known size, end(false) requires every expected byte. The true
    // variant is used only for the explicit unknown-size Update API mode.
    const bool finalized = Update.end(!totalBytesKnown());
    if (!finalized || Update.hasError()) {
        fail(updateError("Firmware verification or finalization failed."), false);
        return false;
    }

    state_ = OTAState::FirmwareStaged;
    lastError_ = String();
    logger_.printf("INFO OTA staging succeeded: %u bytes\n", static_cast<unsigned int>(bytesReceived_));
    runtimeManager_.request(RuntimeAction::RestartDevice);
    logger_.println("INFO OTA requested device restart for staged firmware activation");
    return true;
}

void OTAService::abortUpload(const char* reason) {
    if (!busy()) return;
    fail(reason == nullptr ? String("Firmware upload was aborted.") : String(reason), true);
}

void OTAService::rejectUpload(const char* reason) {
    if (busy() || firmwareStaged()) return;
    fail(reason == nullptr ? String("Invalid firmware upload request.") : String(reason), false);
}

OTAState OTAService::state() const { return state_; }
size_t OTAService::bytesReceived() const { return bytesReceived_; }
size_t OTAService::totalBytes() const { return totalBytes_; }
bool OTAService::totalBytesKnown() const { return totalBytes_ != 0; }
uint8_t OTAService::progressPercent() const {
    if (!totalBytesKnown()) return 0;
    const size_t bounded = bytesReceived_ > totalBytes_ ? totalBytes_ : bytesReceived_;
    return static_cast<uint8_t>((bounded * 100U) / totalBytes_);
}
const String& OTAService::lastError() const { return lastError_; }
bool OTAService::firmwareStaged() const { return state_ == OTAState::FirmwareStaged; }
bool OTAService::restartRequired() const {
    return firmwareStaged() && runtimeManager_.pendingAction() == RuntimeAction::RestartDevice;
}
bool OTAService::busy() const {
    return state_ == OTAState::Uploading || state_ == OTAState::Verifying;
}

void OTAService::fail(const String& reason, bool abortUpdate) {
    if (abortUpdate && Update.isRunning()) Update.abort();
    state_ = OTAState::Failed;
    lastError_ = reason;
    logger_.printf("ERROR OTA upload failed: %s\n", lastError_.c_str());
}

String OTAService::updateError(const char* fallback) const {
    const char* detail = Update.errorString();
    return detail != nullptr && detail[0] != '\0' && Update.hasError()
        ? String(detail) : String(fallback);
}

} // namespace WeatherStation
