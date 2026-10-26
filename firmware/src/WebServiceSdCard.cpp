// EnvNode: see repository LICENSE.
#include "WebService.h"
#include "SdCardWebView.h"
#include "SdCardSpiDiagnostic.h"
#include <new>
#include <esp_system.h>
#include <sys/socket.h>
#include <cerrno>
#include <limits>
#include <cstdio>
namespace EnvNode {
void WebService::handleSdCard() {
    sdFormatTicket_.clear();
    sdDeleteTicket_.clear(); // Returning to the list cancels any outstanding confirmation.
    if (!administrationAvailable()) {
        sendPage(uiText(TextId::SdCard), "/sd-card", "", 503);
        return;
    }
    if(sdFormatResultAvailable_) {
        sdFormatResultAvailable_=false;
        const bool de=configurationService_.getLocale()==Locale::GermanGermany;
        const char* message=sdFormatResult_==SdFormatState::Complete
            ? (de ? "Formatierung erfolgreich. FAT32-Dateisystem und leeres Stammverzeichnis geprüft." : "Format complete. FAT32 filesystem and empty root directory verified.")
            : sdFormatResult_==SdFormatState::MediaChanged
            ? (de ? "Karte wurde geändert. Es wurde nicht formatiert. Bitte erneut bestätigen." : "Card changed. No format performed. Please confirm again.")
            : (de ? "Formatierung fehlgeschlagen. Karte und Systemlog prüfen; Dateisystem möglicherweise unbrauchbar." : "Format failed. Check card and system log; filesystem may be unusable.");
        sendPage(uiText(TextId::SdCard),"/sd-card",String("<section class='card'><p>")+message+"</p><a href='/sd-card'>SD</a></section>",sdFormatResult_==SdFormatState::Complete ? 200 : 503);
        return;
    }
    std::unique_ptr<SdDirectorySnapshot> snapshot(new (std::nothrow) SdDirectorySnapshot);
    if (!snapshot) { server_.send(503, "text/plain", "Insufficient memory"); return; }
    snapshot->path = server_.hasArg("path") ? server_.arg("path") : String("/");
    const String offset = server_.hasArg("offset") ? server_.arg("offset") : String("0");
    bool valid = offset.length()>0 && offset.length()<=4 && validSdDirectoryPath(snapshot->path);
    unsigned parsed=0;
    for (size_t i=0; i<offset.length(); ++i) {
        if (offset[i]<'0' || offset[i]>'9') { valid=false; break; }
        parsed=parsed*10+offset[i]-'0';
    }
    if (parsed>SdDirectoryMaxOffset) valid=false;
    snapshot->offset=parsed;
    if (!valid) snapshot->status=SdDirectoryStatus::InvalidPath;
    else readSdCardDirectory(ModuleSlot::A, currentBoardProfile(), configurationService_.getConfiguration(), actuatorRuntime_, *snapshot);
    if (snapshot->status==SdDirectoryStatus::MountFailed || snapshot->status==SdDirectoryStatus::ReadFailed) {
        logger_.warnf("SD web: %s path=%s heap=%u largest=%u",
            snapshot->status==SdDirectoryStatus::MountFailed ? "mount failed" : "read failed",
            snapshot->path.c_str(), static_cast<unsigned>(ESP.getFreeHeap()),
            static_cast<unsigned>(ESP.getMaxAllocHeap()));
    }
    const int status = snapshot->status==SdDirectoryStatus::InvalidPath ? 400
        : snapshot->status==SdDirectoryStatus::NotFound ? 404 : 200;
    server_.sendHeader("Cache-Control", "no-store");
    sendPage(uiText(TextId::SdCard), "/sd-card", renderSdDirectory(*snapshot, configurationService_.getLocale()), status);
}
}

namespace EnvNode {
namespace {
class SdHttpDownload : public ISdDownloadJob, private ISdDownloadSink {
public:
    SdHttpDownload(std::unique_ptr<ISdStoredFile> file,WiFiClient client,ILogger& logger,uint32_t now)
        : file_(std::move(file)),client_(client),logger_(logger),pump_(*file_,*this,now) {}
    bool service(uint32_t now) override {
        const auto result=pump_.step(now);
        if(result==SdTransferState::More) return true;
        if(result==SdTransferState::Complete) logger_.infof("SD download complete: bytes=%llu",static_cast<unsigned long long>(pump_.sent()));
        else logger_.warnf("SD download aborted: state=%u bytes=%llu",static_cast<unsigned>(result),static_cast<unsigned long long>(pump_.sent()));
        client_.stop(); file_.reset(); return false;
    }
private:
    bool connected() override {return client_.connected();}
    int send(const uint8_t* data,size_t size) override {
        const int result=::send(client_.fd(),data,size,MSG_DONTWAIT);
        if(result<0 && (errno==EAGAIN || errno==EWOULDBLOCK || errno==EINTR)) return 0;
        return result;
    }
    std::unique_ptr<ISdStoredFile> file_; WiFiClient client_; ILogger& logger_; SdDownloadPump pump_;
};
}
void WebService::sdFileError(int code) {
    logger_.warnf("SD file access failed: status=%d heap=%u largest=%u",code,static_cast<unsigned>(ESP.getFreeHeap()),static_cast<unsigned>(ESP.getMaxAllocHeap()));
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    const auto status=static_cast<SdDirectoryStatus>(code);
    const char* message;
    switch(status) {
        case SdDirectoryStatus::InvalidPath: message=de ? "Ungültiger Dateipfad." : "Invalid file path."; break;
        case SdDirectoryStatus::NotFound: message=de ? "Datei nicht gefunden." : "File not found."; break;
        case SdDirectoryStatus::NoCard: message=de ? "Keine Karte erkannt." : "No card detected."; break;
        case SdDirectoryStatus::Conflict: message=de ? "SD-Pins sind anderweitig belegt." : "SD pins are already in use."; break;
        case SdDirectoryStatus::Disabled: message=de ? "SD-Zugriff nicht aktiviert." : "SD access is not enabled."; break;
        default: message=de ? "Dateizugriff fehlgeschlagen. Karte und Systemlog prüfen." : "File access failed. Check card and system log."; break;
    }
    sendPage(uiText(TextId::SdCard),"/sd-card",String("<section class='card'><p>")+message+"</p><a href='/sd-card'>SD</a></section>",status==SdDirectoryStatus::InvalidPath ? 400 : status==SdDirectoryStatus::NotFound ? 404 : 503);
}
void WebService::handleSdDownload() {
    sdFormatTicket_.clear();sdDeleteTicket_.clear();
    if(!administrationAvailable()) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    const String path=server_.arg("path");
    SdDirectoryStatus status;
    auto file=openSdCardFile(ModuleSlot::A,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
    if(!file) {sdFileError(static_cast<int>(status));return;}
    if(file->stamp().bytes>std::numeric_limits<size_t>::max()) {sdFileError(static_cast<int>(SdDirectoryStatus::ReadFailed));return;}
    const size_t size=static_cast<size_t>(file->stamp().bytes);
    std::unique_ptr<ISdDownloadJob> job(new(std::nothrow) SdHttpDownload(std::move(file),server_.client(),logger_,millis()));
    if(!job) {sdFileError(static_cast<int>(SdDirectoryStatus::ReadFailed));return;}
    server_.sendHeader("Cache-Control","no-store");
    server_.sendHeader("X-Content-Type-Options","nosniff");
    server_.sendHeader("Content-Disposition",sdDownloadDisposition(path));
    logger_.infof("SD download started: %s bytes=%llu",path.c_str(),static_cast<unsigned long long>(size));
    server_.setContentLength(size);
    server_.send(200,"application/octet-stream","");
    sdDownload_=std::move(job);
}
void WebService::handleSdDeletePreview() {
    sdFormatTicket_.clear();
    sdDeleteTicket_.clear();
    const String path=server_.arg("path");
    SdDirectoryStatus status;
    auto file=openSdCardFile(ModuleSlot::A,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
    if(!file) {sdFileError(static_cast<int>(status));return;}
    const SdFileStamp stamp=file->stamp(); file.reset();
    char token[33];
    snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
    if(!sdDeleteTicket_.issue(path.c_str(),token,stamp,millis())) {sdFileError(static_cast<int>(SdDirectoryStatus::ReadFailed));return;}
    server_.sendHeader("Cache-Control","no-store");
    sendPage(uiText(TextId::SdCard),"/sd-card",renderSdDeleteConfirmation(path,stamp,token,configurationService_.getLocale()));
}
void WebService::handleSdDelete() {
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    const String path=server_.arg("path");
    SdFileStamp expected;
    const bool ticketValid=sdDeleteTicket_.consume(path.c_str(),server_.arg("token").c_str(),millis(),expected);
    const bool confirmed=administrationAvailable() && ticketValid;
    if(!confirmed) {
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Bestätigung ungültig oder abgelaufen. Datei erneut auswählen.</p>" : "<p>Confirmation invalid or expired. Select the file again.</p>",403);return;
    }
    SdDirectoryStatus status;
    auto file=openSdCardFile(ModuleSlot::A,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
    if(!file) {sdFileError(static_cast<int>(status));return;}
    if(!sameSdFileStamp(expected,file->stamp())) {
        file.reset();
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Datei oder Karte wurde geändert. Bitte erneut bestätigen.</p>" : "<p>File or card changed. Please confirm again.</p>",409);return;
    }
    if(!file->remove()) {
        logger_.warnf("SD delete failed: path=%s step=%s errno=%d",path.c_str(),file->lastError(),file->errorCode());
        file.reset();sdFileError(static_cast<int>(SdDirectoryStatus::ReadFailed));return;
    }
    file.reset();logger_.infof("SD file deleted: %s",path.c_str());
    const String target=sdDirectoryUrl(sdParentPath(path));
    server_.sendHeader("Location",target,true);server_.send(303,"text/plain","See Other");
}
}

namespace EnvNode {
void WebService::handleSdFormatPreview() {
    sdDeleteTicket_.clear();sdFormatTicket_.clear();
    if(!administrationAvailable()) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    SdFileStamp card;SdDirectoryStatus status;
    if(!inspectSdCardForFormat(ModuleSlot::A,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,card,status)) {
        sdFileError(static_cast<int>(status));return;
    }
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    if(!supportedSdFormatCapacity(card.bytes)) {
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Formatieren unterstützt derzeit lesbare Karten von 4 bis 32 GB. Andere Karten bitte am Computer vorbereiten.</p>" : "<p>Formatting currently supports readable 4–32 GB cards. Prepare other cards on a computer.</p>",400);return;
    }
    char token[33];
    snprintf(token,sizeof(token),"%08lx%08lx%08lx%08lx",static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()),static_cast<unsigned long>(esp_random()));
    if(!sdFormatTicket_.issue(token,card,millis())) {sdFileError(static_cast<int>(SdDirectoryStatus::ReadFailed));return;}
    server_.sendHeader("Cache-Control","no-store");
    sendPage(uiText(TextId::SdCard),"/sd-card",renderSdFormatConfirmation(card,token,configurationService_.getLocale()));
}
void WebService::handleSdFormat() {
    SdFileStamp expected;
    const bool valid=sdFormatTicket_.consume(server_.arg("token").c_str(),server_.arg("confirmation").c_str(),millis(),expected);
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    if(!administrationAvailable() || !valid) {
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Bestätigung ungültig oder abgelaufen. Bitte erneut beginnen.</p>" : "<p>Confirmation invalid or expired. Please start again.</p>",403);return;
    }
    SdDirectoryStatus status;
    sdFormat_=startSdCardFormat(ModuleSlot::A,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,expected,status);
    if(!sdFormat_) {sdFileError(static_cast<int>(status));return;}
    sdFormatResultAvailable_=false;
    logger_.infof("SD format requested: slot=A card_bytes=%llu",static_cast<unsigned long long>(expected.bytes));
    server_.sendHeader("Cache-Control","no-store");
    sendPage(uiText(TextId::SdCard),"/sd-card",de
        ? "<section class='card'><p>Formatierung gestartet. Karte eingesteckt und Gerät eingeschaltet lassen. Währenddessen sind Webzugriffe gesperrt. Messungen und Steuerungen laufen weiter.</p><p><a href='/sd-card'>Ergebnis anzeigen</a> (nach einigen Sekunden)</p></section>"
        : "<section class='card'><p>Formatting started. Leave the card inserted and keep power on. Web requests wait during formatting. Measurements and controls continue.</p><p><a href='/sd-card'>Show result</a> (after a few seconds)</p></section>",202);
}
}
