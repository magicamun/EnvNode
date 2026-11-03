// EnvNode: see repository LICENSE.
#include "WebService.h"
#include "SdCardWebView.h"
#include "SdCardSpiDiagnostic.h"
#include "SdStorageArbiter.h"
#include "SdSystemLogService.h"
#include "DiagnosticWebView.h"
#include "HtmlEscaping.h"
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
    if(sdDirectory_) {
        if(!sdDirectory_->done()) {sendSdDirectoryProgress();return;}
        if(server_.arg("listing")==String(sdDirectorySequence_)) {
            const auto& result=sdDirectory_->snapshot();
            const int status=result.status==SdDirectoryStatus::NotFound ? 404 : 200;
            server_.sendHeader("Cache-Control","no-store");
            sendPage(uiText(TextId::SdCard),"/sd-card",renderSdSettings()+renderSdDirectory(result,configurationService_.getLocale()),status);
            sdDirectory_.reset();return;
        }
        sdDirectory_.reset();
    }
    std::unique_ptr<SdDirectorySnapshot> snapshot(new (std::nothrow) SdDirectorySnapshot);
    if (!snapshot) { server_.send(503, "text/plain", "Insufficient memory"); return; }
    snapshot->slot=configurationService_.getConfiguration().sdStorage.slot;
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
    else {
        sdDirectory_=startSdDirectoryRead(snapshot->slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,snapshot->path,snapshot->offset,snapshot->status);
        if(sdDirectory_) {sdDirectoryStarted_=millis();sdDirectorySlowReported_=false;sdDirectoryRequestUrl_=sdDirectoryUrl(snapshot->path,snapshot->offset);++sdDirectorySequence_;sendSdDirectoryProgress();return;}
    }
    if (snapshot->status==SdDirectoryStatus::MountFailed || snapshot->status==SdDirectoryStatus::ReadFailed) {
        logger_.warnf("SD web: %s path=%s heap=%u largest=%u",
            snapshot->status==SdDirectoryStatus::MountFailed ? "mount failed" : "read failed",
            snapshot->path.c_str(), static_cast<unsigned>(ESP.getFreeHeap()),
            static_cast<unsigned>(ESP.getMaxAllocHeap()));
    }
    const int status = snapshot->status==SdDirectoryStatus::InvalidPath ? 400
        : snapshot->status==SdDirectoryStatus::NotFound ? 404 : 200;
    server_.sendHeader("Cache-Control", "no-store");
    sendPage(uiText(TextId::SdCard), "/sd-card", renderSdSettings()+renderSdDirectory(*snapshot, configurationService_.getLocale()), status);
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
        case SdDirectoryStatus::Busy: message=de ? "SD-Karte wird verwendet oder eine Konfigurationsänderung wartet auf Übernahme/Neustart. Bitte erneut versuchen." : "SD card is busy or configuration changes await apply/restart. Please retry."; break;
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
    auto file=openSdCardFile(configurationService_.getConfiguration().sdStorage.slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
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
    auto file=openSdCardFile(configurationService_.getConfiguration().sdStorage.slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
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
    auto file=openSdCardFile(configurationService_.getConfiguration().sdStorage.slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,path,status);
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
    if(!inspectSdCardForFormat(configurationService_.getConfiguration().sdStorage.slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,card,status)) {
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
    sendPage(uiText(TextId::SdCard),"/sd-card",renderSdFormatConfirmation(card,token,configurationService_.getLocale(),moduleSlotName(configurationService_.getConfiguration().sdStorage.slot)));
}
void WebService::handleSdFormat() {
    SdFileStamp expected;
    const bool valid=sdFormatTicket_.consume(server_.arg("token").c_str(),server_.arg("confirmation").c_str(),millis(),expected);
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    if(!administrationAvailable() || !valid) {
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Bestätigung ungültig oder abgelaufen. Bitte erneut beginnen.</p>" : "<p>Confirmation invalid or expired. Please start again.</p>",403);return;
    }
    SdDirectoryStatus status;
    sdFormat_=startSdCardFormat(configurationService_.getConfiguration().sdStorage.slot,currentBoardProfile(),configurationService_.getConfiguration(),actuatorRuntime_,expected,status);
    if(!sdFormat_) {sdFileError(static_cast<int>(status));return;}
    sdFormatResultAvailable_=false;
    logger_.infof("SD format requested: slot=%s card_bytes=%llu",moduleSlotName(configurationService_.getConfiguration().sdStorage.slot),static_cast<unsigned long long>(expected.bytes));
    server_.sendHeader("Cache-Control","no-store");
    sendPage(uiText(TextId::SdCard),"/sd-card",de
        ? "<section class='card'><p>Formatierung gestartet. Karte eingesteckt und Gerät eingeschaltet lassen. Währenddessen sind Webzugriffe gesperrt. Messungen und Steuerungen laufen weiter.</p><p><a href='/sd-card'>Ergebnis anzeigen</a> (nach einigen Sekunden)</p></section>"
        : "<section class='card'><p>Formatting started. Leave the card inserted and keep power on. Web requests wait during formatting. Measurements and controls continue.</p><p><a href='/sd-card'>Show result</a> (after a few seconds)</p></section>",202);
}
}

namespace EnvNode {
String WebService::renderSdSettings() {
    const auto& sd=configurationService_.getConfiguration().sdStorage;
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    String html="<section class='card'><h2>";
    html+=de ? "SD-Einstellungen" : "SD settings";
    html+="</h2>";
    if(sd.automatic || sd.moduleConflict) html+=sd.moduleConflict ? (de ? "<p>SD-Modulkonflikt oder nicht unterstützter Modulvertrag; Zugriff gesperrt.</p>" : "<p>SD module conflict or unsupported contract; access blocked.</p>") : (de ? "<p>Slot und SD-Zugriff werden automatisch vom Modul-EEPROM übernommen.</p>" : "<p>Slot and SD access are selected automatically from the module EEPROM.</p>");
    html+="<form method='post' action='/sd-card/settings'><label><input type='checkbox' name='enabled' value='1'";
    if(sd.enabled) html+=" checked";
    if(sd.automatic) html+=" disabled";
    html+=">";html+=de ? "SD-Zugriff aktivieren" : "Enable SD access";
    html+="</label><label>Slot <select name='slot'";
    if(sd.automatic) html+=" disabled";
    html+="><option value='0'";
    if(sd.slot==ModuleSlot::A) html+=" selected";
    html+=">A</option><option value='1'";
    if(sd.slot==ModuleSlot::B) html+=" selected";
    html+=">B</option></select></label><label><input type='checkbox' name='systemLog' value='1'";
    if(sd.systemLog) html+=" checked";
    html+=">";html+=de ? "Systemlog auf SD aufzeichnen" : "Record system log on SD";
    html+="</label><label><input type='checkbox' name='systemDiagnostics' value='1'";
    if(sd.systemDiagnostics)html+=" checked";if(!extendedDiagnosticsAvailable())html+=" disabled";
    html+=">";html+=de?"Erweiterte Systemdiagnose":"Extended system diagnostics";
    html+="<input type='hidden' name='systemDiagnosticsEditable' value='";html+=extendedDiagnosticsAvailable()?"1":"0";html+="'>";
    html+="</label><button type='submit'>";html+=de ? "Speichern" : "Save";
    html+="</button></form><p>";html+=de ? "Die Slot-Auswahl gilt für das Breakout ohne EEPROM-Erkennung." : "Slot selection supports the breakout without EEPROM detection.";
    html+="</p><p>Systemlog: "+escapeHtml(sdSystemLogStatus())+"</p>";
    html+="<p>";html+=de?"Erweiterte Diagnose: ":"Extended diagnostics: ";html+=escapeHtml(extendedDiagnosticsStatus())+"</p>";
    if(sd.systemDiagnostics && extendedDiagnosticsAvailable()) {
        html+="<form method='post' action='/sd-card/diagnostic-test'><button type='submit'>";
        html+=de?"Diagnose-Testereignis aufzeichnen":"Record diagnostic test event";html+="</button></form>";
    }
    if(sd.enabled) {
        html+="<form method='post' action='/sd-card/test'><button type='submit'>";
        html+=de ? "4-KiB-Schreib-/Lesetest starten" : "Run 4 KiB write/read test";
        html+="</button></form><p><a href='/sd-card/rotation-test'>";
        html+=de ? "Rotation und Speicherbudget testen" : "Test rotation and storage budget";
        html+="</a></p>";
    }
    return html+"</section>";
}
void WebService::handleSdSettings() {
    sdDeleteTicket_.clear();sdFormatTicket_.clear();
    if(!administrationAvailable()) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    const auto& current=configurationService_.getConfiguration().sdStorage;
    const String slot=current.automatic ? (current.slot==ModuleSlot::A ? String("0") : String("1")) : server_.arg("slot");
    if(slot!="0" && slot!="1") {server_.send(400,"text/plain","Invalid SD slot");return;}
    SdStorageConfiguration requested;
    requested.enabled=server_.hasArg("enabled");requested.slot=slot=="1" ? ModuleSlot::B : ModuleSlot::A;
    requested.systemLog=server_.hasArg("systemLog");
    requested.systemDiagnostics=extendedDiagnosticsChoice(current.systemDiagnostics,extendedDiagnosticsAvailable(),server_.arg("systemDiagnosticsEditable")=="1",server_.hasArg("systemDiagnostics"));
    SdStorageLease lease;
    if(!lease) {server_.send(503,"text/plain","SD busy; retry settings save");return;}
    if(!configurationService_.setSdStorageConfiguration(requested)) {
        const bool de=configurationService_.getLocale()==Locale::GermanGermany;
        sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>SD-Einstellungen nicht gespeichert. Slot und GPIO-Konflikte prüfen.</p><a href='/sd-card'>SD</a>" : "<p>SD settings not saved. Check slot and GPIO conflicts.</p><a href='/sd-card'>SD</a>",400);return;
    }
    configureSdSystemLog(configurationService_.getConfiguration(),actuatorRuntime_);
    logger_.infof("SD settings saved: enabled=%u slot=%s system_log=%u",requested.enabled,moduleSlotName(requested.slot),requested.systemLog);
    server_.sendHeader("Location","/sd-card",true);server_.send(303,"text/plain","See Other");
}
void WebService::handleSdTest() {
    sdDeleteTicket_.clear();sdFormatTicket_.clear();
    if(!administrationAvailable() || !configurationService_.getConfiguration().sdStorage.enabled) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    // Reuse the runtime occupancy preflight, including automatic module actuators.
    SdFileStamp card;SdDirectoryStatus status;
    const auto& configuration=configurationService_.getConfiguration();
    if(!inspectSdCardForFormat(configuration.sdStorage.slot,currentBoardProfile(),configuration,actuatorRuntime_,card,status)) {sdFileError(static_cast<int>(status));return;}
    runSdCardSpiDiagnostic(configuration.sdStorage.slot,currentBoardProfile(),configuration,nullptr,0,logger_);
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    sendPage(uiText(TextId::SdCard),"/sd-card",de ? "<p>Test beendet. Ergebnis im Systemlog prüfen.</p><a href='/sd-card'>SD</a>" : "<p>Test finished. Check the system log for the result.</p><a href='/sd-card'>SD</a>");
}
}

namespace EnvNode {
void WebService::sendSdDirectoryProgress() {
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    // The request URL belongs to the web loop, never to the running worker.
    const String url=sdDirectoryRequestUrl_+"&listing="+String(sdDirectorySequence_);
    String html="<section class='card'><p>";
    html+=de ? "SD-Verzeichnis wird gelesen. Andere Seiten bleiben erreichbar." : "Reading SD directory. Other pages remain available.";
    html+="</p><a id='sd-progress-link' href='"+escapeHtml(url)+"'>";
    html+=de ? "Ergebnis anzeigen" : "Show result";
    html+="</a></section><script>setTimeout(function(){location.replace(document.getElementById('sd-progress-link').href);},1000);</script>";
    server_.sendHeader("Cache-Control","no-store");
    sendPage(uiText(TextId::SdCard),"/sd-card",html,202);
}
}

namespace EnvNode {
void WebService::handleSdRotationPreview() {
    if(!administrationAvailable()) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    const bool de=configurationService_.getLocale()==Locale::GermanGermany;
    server_.sendHeader("Cache-Control","no-store");
    const bool progress=server_.arg("progress")=="1";
    if(sdRotationTest_) {
        if(sdRotationTest_->done()) {
            const String result=renderSdRotationResult(sdRotationTest_->result(),configurationService_.getLocale());
            if(progress) server_.send(200,"text/html; charset=utf-8",result);
            else sendPage(uiText(TextId::SdCard),"/sd-card",result);
            return;
        }
        if(progress) server_.send(202,"text/plain","Running");
        else sendPage(uiText(TextId::SdCard),"/sd-card",renderSdRotationProgress(configurationService_.getLocale()),202);
        return;
    }
    if(progress) {server_.send(409,"text/plain","No rotation test");return;}
    String html="<section class='card'><h2>";
    html+=de ? "SD-Rotationstest" : "SD rotation test";
    html+="</h2><p>";
    html+=de ? "Prüft Dateiwechsel, Speicherbudget und Dateiinhalte mit kleinen Testgrenzen. Erstellt etwa 40 KiB Testdaten in einem neuen, eigenen Verzeichnis und räumt diese anschließend auf. Bestehende Systemlogs bleiben erhalten. Karte während des Tests eingesteckt lassen." : "Checks file rotation, storage budget and file contents with small test limits. Creates about 40 KiB in a new dedicated directory and cleans it up afterward. Existing system logs are preserved. Leave the card inserted during the test.";
    html+="</p><form method='post' action='/sd-card/rotation-test'><button type='submit'>";
    html+=de ? "Rotationstest starten" : "Start rotation test";
    html+="</button></form><a href='/sd-card'>SD</a></section>";
    sendPage(uiText(TextId::SdCard),"/sd-card",html);
}
void WebService::handleSdRotationStart() {
    if(!administrationAvailable()) {sdFileError(static_cast<int>(SdDirectoryStatus::Disabled));return;}
    sdDeleteTicket_.clear();sdFormatTicket_.clear();
    if(sdRotationTest_ && !sdRotationTest_->done()) {handleSdRotationPreview();return;}
    sdRotationTest_.reset();
    SdDirectoryStatus status;
    const auto& configuration=configurationService_.getConfiguration();
    sdRotationTest_=startSdRotationTest(configuration.sdStorage.slot,currentBoardProfile(),configuration,actuatorRuntime_,status);
    if(!sdRotationTest_) {sdFileError(static_cast<int>(status));return;}
    sdRotationTestReported_=false;
    logger_.info("SD rotation test requested: isolated directory; system logs preserved");
    server_.sendHeader("Location","/sd-card/rotation-test",true);server_.send(303,"text/plain","See Other");
}
}
