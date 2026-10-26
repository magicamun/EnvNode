#include "WebResult.h"
// EnvNode: see repository LICENSE.
#include "FirmwareWebRoutes.h"
#include "HtmlEscaping.h"
#include <cstring>
namespace EnvNode {
namespace {
const char* webOtaStateName(OTAState state, Locale locale) {
    switch (state) {
        case OTAState::Idle: return webText(locale, TextId::Idle);
        case OTAState::Uploading: return webText(locale, TextId::Uploading);
        case OTAState::Verifying: return webText(locale, TextId::Verifying);
        case OTAState::FirmwareStaged: return webText(locale, TextId::FirmwareStaged);
        case OTAState::Failed: return webText(locale, TextId::Failed);
        default: return webText(locale, TextId::Unknown);
    }
}
String badge(const char* text, const char* style) {
    return "<span class='badge " + String(style) + "'>" + text + "</span>";
}
}
FirmwareWebRoutes::FirmwareWebRoutes(WebServer& server, IWebPagePresenter& pages,
    OTAService& ota, IDeviceRestart& restart, FirmwareWebBuildInfo build,
    ResetConfiguration resetConfiguration, FormatNumber formatNumber)
    : server_(server), pages_(pages), otaService_(ota), deviceRestart_(restart),
      build_(build), resetConfiguration_(resetConfiguration), formatNumber_(formatNumber) {}
void FirmwareWebRoutes::registerRoutes(WebServer& server) {
    server.on("/firmware", HTTP_GET, [this]() { handleFirmware(); });
    server.on("/firmware/upload", HTTP_POST,
        [this]() { handleFirmwareUpload(); }, [this]() { handleFirmwareUploadData(); });
    server.on("/restart", HTTP_POST, [this]() { handleRestart(); });
    server.on("/factory-reset", HTTP_POST, [this]() { handleFactoryReset(); });
}
const char* FirmwareWebRoutes::uiText(TextId id) const { return webText(pages_.webLocale(), id); }
void FirmwareWebRoutes::sendPage(const char* title, const char* route, const String& content, int status) {
    pages_.sendPage(title, route, content, status);
}

String FirmwareWebRoutes::otaStatusHtml() const {
    String html;
    html.reserve(520);
    html = (String("<div class='kv'><span>") + uiText(TextId::OTAState) + "</span><span>");
    html += webOtaStateName(otaService_.state(), pages_.webLocale());
    html += (String("</span><span>") + uiText(TextId::UploadStatus) + "</span><span>");
    if (otaService_.totalBytesKnown()) {
        html += formatNumber_(otaService_.bytesReceived(), 0);
        html += " / ";
        html += formatNumber_(otaService_.totalBytes(), 0);
        html += (String(" ") + uiText(TextId::Bytes2));
        html += formatNumber_(otaService_.progressPercent(), 0);
        html += "%)";
    } else if (otaService_.bytesReceived() > 0) {
        html += formatNumber_(otaService_.bytesReceived(), 0);
        html += (String(" ") + uiText(TextId::BytesReceived));
    } else {
        html += "—";
    }
    html += (String("</span><span>") + uiText(TextId::RestartRequired) + "</span><span>");
    html += uiText(otaService_.restartRequired() ? TextId::Yes : TextId::No);
    html += "</span>";
    if (otaService_.state() == OTAState::Failed && !otaService_.lastError().isEmpty()) {
        html += (String("<span>") + uiText(TextId::LastError) + "</span><span>");
        html += escapeHtml(otaService_.lastError());
        html += "</span>";
    }
    html += "</div>";
    return html;
}

void FirmwareWebRoutes::handleFirmware() {
    String c;
    c.reserve(2000);
    const char* sourceStyle = build_.SourceDirty ? "warn"
        : String(build_.SourceState) == "clean" ? "good" : "warn";
    c = (String("<section class='card'><h2>") + uiText(TextId::RunningFirmware) + "</h2><div class='kv'><span>" + uiText(TextId::FirmwareVersion) + "</span><span>") + String(build_.SemanticVersion) + (String("</span><span>") + uiText(TextId::BuildIdentity) + "</span><span>") + build_.CompactIdentity + (String("</span><span>") + uiText(TextId::BuildNumber) + "</span><span>") + build_.BuildNumber + (String("</span><span>") + uiText(TextId::GitCommit) + "</span><span>") + build_.GitCommit + (String("</span><span>") + uiText(TextId::GitBranch) + "</span><span>") + escapeHtml(build_.GitBranch) + (String("</span><span>") + uiText(TextId::BuildTimestamp) + "</span><span>") + build_.BuildTimestampUtc + (String("</span><span>") + uiText(TextId::SourceState) + "</span><span>") + badge(build_.SourceDirty ? uiText(TextId::Dirty) : String(build_.SourceState) == "clean" ? uiText(TextId::Clean) : uiText(TextId::Unknown), sourceStyle) + (String("</span><span>") + uiText(TextId::StagedFirmware) + "</span><span>")+(otaService_.firmwareStaged()?uiText(TextId::ReadyForActivation):"—")+"</span></div>";
    if (otaService_.firmwareStaged()) c += (String("<p class='help'>") + uiText(TextId::StagedFirmwareVersionMetadataIsNotYetAvailable) + "</p>");
    c += (String("</section><section class='card'><h2>") + uiText(TextId::FirmwareUpdateStatus) + "</h2>") + otaStatusHtml() + "</section>";
    if (!otaService_.firmwareStaged() && !otaService_.busy()) {
        c += (String("<section class='card'><h2>") + uiText(TextId::UploadFirmware) + (String("</h2><p class='help'>") + uiText(TextId::BuildWith) + " " + "<code>pio run</code>" + uiText(TextId::ThenUpload) + " " + "<code>.pio/build/&lt;environment&gt;/firmware.bin</code>.</p><form method='post' action='/firmware/upload' enctype='multipart/form-data' onsubmit='document.getElementById(\"firmwareSize\").value=document.getElementById(\"firmwareFile\").files[0].size'><input type='hidden' id='firmwareSize' name='firmwareSize' value='0'><label>" + uiText(TextId::FirmwareBinary) + "<input id='firmwareFile' type='file' name='firmware' accept='.bin,application/octet-stream' required></label><div class='actions'><button>") + uiText(TextId::UploadFirmware) + "</button></div></form></section>");
    }
    c += (String("<section class='card'><h2>") + uiText(TextId::Restart) + (String("</h2><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>") + uiText(TextId::RestartNow) + "</button></form></section><section class='card'><h2>") + uiText(TextId::FactoryReset) + (String("</h2><p>") + uiText(TextId::ThisPermanentlyRemovesAllStoredConfigurationAndRestarts) + "</p><form method='post' action='/factory-reset' onsubmit='return confirm(globalThis.EnvNodeUi.EraseALLConfigurationAndRestart)'><button class='danger'>") + uiText(TextId::FactoryReset) + "</button></form></section>");
    sendPage(uiText(TextId::Firmware), "/firmware", c);
}

void FirmwareWebRoutes::handleFirmwareUploadData() {
    HTTPUpload& upload = server_.upload();
    if (upload.status == UPLOAD_FILE_START) {
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = String();
        if (otaService_.busy()) {
            firmwareUploadRequestError_ = uiText(TextId::AnotherFirmwareUploadIsAlreadyActive);
            return;
        }
        if (otaService_.firmwareStaged()) {
            firmwareUploadRequestError_ = uiText(TextId::FirmwareIsAlreadyStagedAndAwaitingRestart);
            return;
        }
        if (upload.name != "firmware" || upload.filename.isEmpty()
            || !upload.filename.endsWith(".bin")) {
            firmwareUploadRequestError_ = uiText(TextId::SelectAValidPlatformIOFirmwareBinFile);
            otaService_.rejectUpload(firmwareUploadRequestError_.c_str());
            return;
        }
        const size_t expectedSize = static_cast<size_t>(server_.arg("firmwareSize").toInt());
        firmwareUploadRequestAccepted_ = otaService_.beginUpload(expectedSize);
        if (!firmwareUploadRequestAccepted_) {
            firmwareUploadRequestError_ = otaService_.lastError().isEmpty()
                ? String(uiText(TextId::FirmwareUploadCouldNotBeStarted)) : otaService_.lastError();
        }
        return;
    }
    if (!firmwareUploadRequestAccepted_) return;
    if (upload.status == UPLOAD_FILE_WRITE) {
        if (!otaService_.writeChunk(upload.buf, upload.currentSize)) {
            firmwareUploadRequestAccepted_ = false;
            firmwareUploadRequestError_ = otaService_.lastError();
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (!otaService_.finishUpload()) {
            firmwareUploadRequestAccepted_ = false;
            firmwareUploadRequestError_ = otaService_.lastError();
        }
    } else if (upload.status == UPLOAD_FILE_ABORTED) {
        otaService_.abortUpload(uiText(TextId::FirmwareUploadWasAbortedBeforeCompletion));
        firmwareUploadRequestAccepted_ = false;
        firmwareUploadRequestError_ = otaService_.lastError();
    }
}

void FirmwareWebRoutes::handleFirmwareUpload() {
    if (firmwareUploadRequestAccepted_ && otaService_.firmwareStaged()) {
        String content;
        content.reserve(520);
        content = (String("<div class='notice success'><strong>") + uiText(TextId::FirmwareUploadedSuccessfully) + "</strong><p>" + uiText(TextId::FirmwareIsStaged) + "</p><p>" + uiText(TextId::DeviceRestartIsRequiredToActivateTheNew) + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>" + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/firmware'>" + uiText(TextId::RestartLater) + "</a>");
        sendPage(uiText(TextId::FirmwareStaged), "/firmware", content);
        return;
    }
    const String reason = !firmwareUploadRequestError_.isEmpty()
        ? firmwareUploadRequestError_ : otaService_.lastError();
    String content;
    content.reserve(480 + reason.length());
    content = (String("<div class='notice error'><strong>") + uiText(TextId::FirmwareUpdateFailed2) + "</strong><p>");
    content += escapeHtml(reason.isEmpty() ? String(uiText(TextId::InvalidFirmwareUploadRequest)) : reason);
    content += (((String("</p><p>") + uiText(TextId::TheCurrentlyRunningFirmwareRemainsActive) + "</p></div><a class='button' href='/firmware'>")) + uiText(TextId::Back) + "</a>");
    sendPage(uiText(TextId::FirmwareUpdateFailed), "/firmware", content, 400);
}

void FirmwareWebRoutes::handleRestart() { if(otaService_.busy()){sendResult(uiText(TextId::RestartUnavailable),"/firmware",uiText(TextId::AFirmwareUploadIsCurrentlyActive),false);return;} deviceRestart_.requestDeviceRestart(); sendResult(uiText(TextId::Restarting),"/firmware",uiText(TextId::TheDeviceIsRestartingNow),true); performExplicitRestart(); }

void FirmwareWebRoutes::handleFactoryReset() { if(otaService_.busy()){sendResult(uiText(TextId::FactoryResetUnavailable),"/firmware",uiText(TextId::AFirmwareUploadIsCurrentlyActive),false);return;} if(!resetConfiguration_()){sendResult(uiText(TextId::FactoryResetFailed),"/firmware",uiText(TextId::StoredConfigurationCouldNotBeCleared),false);return;} deviceRestart_.requestDeviceRestart(); sendResult(uiText(TextId::FactoryResetComplete),"/firmware",uiText(TextId::ConfigurationErasedRestartingIntoProvisioningMode),true);performExplicitRestart(); }

void FirmwareWebRoutes::performExplicitRestart() { server_.client().flush(); deviceRestart_.performDeviceRestart(); }
}

namespace EnvNode {
void FirmwareWebRoutes::sendResult(const char* title, const char* route, const char* message, bool success) { sendWebResult(pages_, title, route, message, success); }
}
