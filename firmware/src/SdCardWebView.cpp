// EnvNode: see repository LICENSE.
#include "SdCardWebView.h"
#include "HtmlEscaping.h"
#include <cstdio>
namespace EnvNode {
bool validSdDirectoryPath(const String& path) {
    if (path.length() == 0 || path.length() > 240 || path[0] != '/') return false;
    if (path == "/") return true;
    if (path[path.length()-1] == '/') return false;
    size_t start = 1;
    for (size_t i = 1; i <= path.length(); ++i) {
        if (i < path.length()) {
            const unsigned char c = path[i];
            if (c < 32 || c == 127 || c == '\\') return false;
        }
        if (i == path.length() || path[i] == '/') {
            const size_t size = i-start;
            if (size == 0 || (size == 1 && path[start] == '.')
                || (size == 2 && path[start] == '.' && path[start+1] == '.')) return false;
            start = i + 1;
        }
    }
    return true;
}
String sdEncodeComponent(const String& value) {
    const char* hex="0123456789ABCDEF";
    String encoded;
    for(size_t i=0;i<value.length();++i) {
        const unsigned char c=value[i];
        if((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='-' || c=='_' || c=='.') encoded+=static_cast<char>(c);
        else {encoded+='%';encoded+=hex[c>>4];encoded+=hex[c&15];}
    }
    return encoded;
}
String sdDownloadDisposition(const String& path) {
    size_t start=0;
    for(size_t i=0;i<path.length();++i) if(path[i]=='/') start=i+1;
    String name,ascii;
    for(size_t i=start;i<path.length();++i) {
        const unsigned char c=path[i];name+=static_cast<char>(c);
        ascii+=((c>='a'&&c<='z') || (c>='A'&&c<='Z') || (c>='0'&&c<='9') || c=='.' || c=='_' || c=='-') ? static_cast<char>(c) : '_';
    }
    if(ascii.length()==0) ascii="sd-download.bin";
    return "attachment; filename=\""+ascii+"\"; filename*=UTF-8''"+sdEncodeComponent(name);
}
String sdDirectoryUrl(const String& path,unsigned offset) {
    String url="/sd-card?path="+sdEncodeComponent(path);
    if(offset) {url+="&offset=";url+=String(offset);} return url;
}
String sdParentPath(const String& path) {
    size_t last=0;
    for(size_t i=1;i<path.length();++i) if(path[i]=='/') last=i;
    String parent="/";
    for(size_t i=1;i<last;++i) parent+=path[i];
    return parent;
}

namespace {
String bytes(uint64_t value) {
    char buffer[32]; snprintf(buffer, sizeof(buffer), "%llu", static_cast<unsigned long long>(value));
    return String(buffer);
}
}
String renderSdFormatConfirmation(const SdFileStamp& card,const String& token,Locale locale,const char* slot) {
    const bool de=locale==Locale::GermanGermany;
    String html="<section class='card'><h2>";
    html+=de ? "SD-Karte formatieren" : "Format SD card";
    html+="</h2><p>Slot "+escapeHtml(slot)+" · "+bytes(card.bytes)+" Bytes</p><p><strong>";
    html+=de ? "Alle Dateien und Partitionen auf dieser Karte werden gelöscht." : "All files and partitions on this card will be deleted.";
    html+="</strong></p><p>";
    html+=de ? "Die Karte wird als eine FAT32-Partition neu eingerichtet. Schnellformatierung, kein sicheres Löschen. Währenddessen Karte nicht entfernen, Gerät nicht ausschalten und keine Firmware aktualisieren. Bestätigung gilt 2 Minuten." : "The card will be recreated as one FAT32 partition. Quick format, not secure erasure. Do not remove the card, power off or update firmware during formatting. Confirmation expires after 2 minutes.";
    html+="</p><form method='post' action='/sd-card/format'><input type='hidden' name='token' value='"+escapeHtml(token)+"'><label>";
    html+=de ? "Zur Bestätigung FORMATIEREN eingeben" : "Type FORMATIEREN to confirm";
    html+="<input name='confirmation' required autocomplete='off' pattern='FORMATIEREN'></label><button type='submit'>";
    html+=de ? "Karte endgültig formatieren" : "Permanently format card";
    html+="</button></form><a href='/sd-card'>";
    html+=de ? "Abbrechen" : "Cancel";
    return html+"</a></section>";
}
String renderSdDirectory(const SdDirectorySnapshot& s, Locale locale) {
    const bool de = locale == Locale::GermanGermany;
    String html;
    if (!html.reserve(12288)) return de ? String("<p>Zu wenig Speicher. Bitte erneut versuchen.</p>") : String("<p>Insufficient memory. Please retry.</p>");
    html = "<section class='card'><h2>";
    html += de ? "SD-Karte — Slot " : "SD card — Slot ";
    html += moduleSlotName(s.slot);
    html += "</h2><p>";
    if (s.status != SdDirectoryStatus::Ready) {
        switch (s.status) {
            case SdDirectoryStatus::Disabled: html += de ? "SD-Zugriff ist in diesem Firmware-Build nicht aktiviert." : "SD access is not enabled in this firmware build."; break;
            case SdDirectoryStatus::Busy: html += de ? "SD-Karte wird verwendet oder eine Konfigurationsänderung wartet auf Übernahme/Neustart. Bitte erneut versuchen." : "SD card is busy or configuration changes await apply/restart. Please retry."; break;
            case SdDirectoryStatus::NoCard: html += de ? "Keine Karte erkannt." : "No card detected."; break;
            case SdDirectoryStatus::Conflict: html += de ? "Slot nicht verfügbar oder Pins anderweitig belegt." : "Slot unavailable or pins already in use."; break;
            case SdDirectoryStatus::MountFailed: html += de ? "Karte konnte nicht eingebunden werden. Bitte erneut versuchen und Systemlog prüfen." : "Could not mount card. Please retry and check the system log."; break;
            case SdDirectoryStatus::NotFound: html += de ? "Verzeichnis nicht gefunden." : "Directory not found."; break;
            case SdDirectoryStatus::InvalidPath: html += de ? "Ungültiger Verzeichnispfad oder Seitenbereich." : "Invalid directory path or page range."; break;
            default: html += de ? "Lesefehler oder Karte entfernt. Bitte erneut versuchen." : "Read error or card removed. Please retry."; break;
        }
        html += "</p><a href='/sd-card'>";
        html += de ? "Aktualisieren / Stammverzeichnis" : "Refresh / root directory";
        return html + "</a></section>";
    }
    html += de ? "Karte lesbar" : "Card readable";
    html += "</p><div class='kv'><span>";
    html += de ? "Kartentyp" : "Card type";
    html += "</span><span>" + escapeHtml(s.info.type) + "</span><span>";
    html += de ? "Kartenkapazität (Bytes)" : "Card capacity (bytes)";
    html += "</span><span>" + bytes(s.info.cardBytes) + "</span><span>";
    html += de ? "Dateisystem (Bytes)" : "Filesystem (bytes)";
    html += "</span><span>" + bytes(s.info.filesystemBytes) + "</span><span>";
    html += de ? "Belegt (Bytes)" : "Used (bytes)";
    const String unavailable=de ? "Nicht verfügbar" : "Unavailable";
    const String estimate=s.info.spaceUsageEstimated ? String("≈ ") : String();
    html += "</span><span>" + (s.info.spaceUsageKnown ? estimate+bytes(s.info.usedBytes) : unavailable) + "</span><span>";
    html += de ? "Frei (Bytes)" : "Free (bytes)";
    html += "</span><span>" + (s.info.spaceUsageKnown ? estimate+bytes(s.info.filesystemBytes >= s.info.usedBytes ? s.info.filesystemBytes-s.info.usedBytes : 0) : unavailable) + "</span></div></section>";
    html += "<section class='card'><h2 style='overflow-wrap:anywhere'>" + escapeHtml(s.path) + "</h2><p><a href='" + escapeHtml(sdDirectoryUrl(s.path, s.offset)) + "'>";
    html += de ? "Aktualisieren" : "Refresh";
    html += "</a> · <a href='/sd-card'>/</a>";
    if (s.path != "/") {
        size_t last=0;
        for (size_t i=1; i<s.path.length(); ++i) if (s.path[i]=='/') last=i;
        String parent="/";
        for (size_t i=1; i<last; ++i) parent+=s.path[i];
        html += " · <a href='" + escapeHtml(sdDirectoryUrl(parent)) + "'>..</a>";
    }
    html += " · <a href='/sd-card/format'>";
    html += de ? "Karte formatieren" : "Format card";
    html += "</a></p><p class='help'>";
    html += de ? "Downloads laufen einzeln. Zeitlimit: 10 Minuten." : "Downloads run one at a time. Time limit: 10 minutes.";
    html += "</p><div class='table-scroll'><table><thead><tr><th>Name</th><th>";
    html += de ? "Typ" : "Type";
    html += "</th><th>Bytes</th><th>";
    html += de ? "Aktionen" : "Actions";
    html += "</th></tr></thead><tbody>";
    size_t renderedCount=0;
    for (size_t i=0; i<s.count && i<SdDirectoryPageSize; ++i) {
        const auto& entry = s.entries[i];
        String row;
        const String path = s.path == "/" ? s.path+entry.name : s.path+"/"+entry.name;
        row += "<tr><td>";
        if (entry.directory && validSdDirectoryPath(path)) row += "<a href='" + escapeHtml(sdDirectoryUrl(path)) + "'>" + escapeHtml(entry.name) + "/</a>";
        else row += escapeHtml(entry.name);
        row += "</td><td>";
        row += entry.directory ? (de ? "Verzeichnis" : "Directory") : (de ? "Datei" : "File");
        row += "</td><td>" + (entry.directory ? String("—") : bytes(entry.bytes)) + "</td><td>";
        if(!entry.directory && validSdDirectoryPath(path)) {
            const String query=escapeHtml(sdEncodeComponent(path));
            row += "<a href='/sd-card/download?path="+query+"'>Download</a> · <a href='/sd-card/delete?path="+query+"'>";
            row += de ? "Löschen" : "Delete";
            row += "</a>";
        }
        row += "</td></tr>";
        if (html.length()+row.length()>11264) break;
        html += row;
        ++renderedCount;
    }
    html += "</tbody></table></div>";
    if (s.count == 0) { html += "<p>"; html += de ? "Keine Einträge auf dieser Seite." : "No entries on this page."; html += "</p>"; }
    if (s.offset) {
        html += "<a href='" + escapeHtml(sdDirectoryUrl(s.path, s.offset>=SdDirectoryPageSize ? s.offset-SdDirectoryPageSize : 0)) + "'>";
        html += de ? "Zurück" : "Previous"; html += "</a> ";
    }
    if ((s.more || renderedCount<s.count) && renderedCount>0 && !s.limited && s.offset+renderedCount<=SdDirectoryMaxOffset) {
        html += "<a href='" + escapeHtml(sdDirectoryUrl(s.path, s.offset+renderedCount)) + "'>";
        html += de ? "Weiter" : "Next"; html += "</a>";
    }
    if(s.timedOut) {
        html+="<p>";
        html+=de ? "Lesezeit für diese Seite erreicht. Weitere Einträge mit Weiter anzeigen; falls keine Einträge gelesen wurden, bitte aktualisieren." : "Read time limit reached for this page. Use Next for more entries; refresh if no entries were read.";
        html+="</p>";
    }
    if (s.limited) { html += "<p>"; html += de ? "Anzeigegrenze erreicht. Bitte kleinere Unterverzeichnisse verwenden." : "Listing limit reached. Please use smaller subdirectories."; html += "</p>"; }
    return html + "</section>";
}
}

namespace EnvNode {
String renderSdDeleteConfirmation(const String& path,const SdFileStamp& stamp,const String& token,Locale locale) {
    const bool de=locale==Locale::GermanGermany;
    String html="<section class='card'><h2>";
    html+=de ? "Datei löschen?" : "Delete file?";
    html+="</h2><p>"+escapeHtml(path)+"</p><p>"+bytes(stamp.bytes)+" Bytes</p><p>";
    html+=de ? "Diese Datei wird dauerhaft entfernt." : "This file will be permanently removed.";
    html+="</p><p>";
    html+=de ? "Die Bestätigung gilt zwei Minuten." : "Confirmation is valid for two minutes.";
    html+="</p><form method='post' action='/sd-card/delete'><input type='hidden' name='path' value='"+escapeHtml(path)+"'><input type='hidden' name='token' value='"+escapeHtml(token)+"'><button type='submit'>";
    html+=de ? "Datei endgültig löschen" : "Permanently delete file";
    html+="</button></form><p><a href='"+escapeHtml(sdDirectoryUrl(sdParentPath(path)))+"'>";
    html+=de ? "Abbrechen" : "Cancel";
    return html+"</a></p></section>";
}
}

namespace EnvNode {
String renderSdRotationProgress(Locale locale) {
    const bool de=locale==Locale::GermanGermany;
    String html="<section class='card' id='sd-rotation-result' aria-live='polite'><p>";
    html+=de ? "Rotationstest läuft. Karte eingesteckt lassen. Andere Seiten bleiben erreichbar." : "Rotation test running. Leave the card inserted. Other pages remain available.";
    html+="</p><a href='/sd-card/rotation-test'>";
    html+=de ? "Ergebnis anzeigen" : "Show result";
    html+="</a></section>";
    // Keep the current page visible while polling; never navigate during a response.
    html+="<script>(()=>{const target=document.getElementById('sd-rotation-result');async function poll(){const controller=new AbortController();const timeout=setTimeout(()=>controller.abort(),5000);let complete=false;try{const response=await fetch('/sd-card/rotation-test?progress=1',{cache:'no-store',signal:controller.signal});if(response.status===200){const html=await response.text();if(html){target.outerHTML=html;complete=true}}}catch(error){}finally{clearTimeout(timeout);if(!complete)setTimeout(poll,2000)}}setTimeout(poll,2000)})();</script>";
    return html;
}
String renderSdRotationResult(const SdRotationTestResult& r,Locale locale) {
    const bool de=locale==Locale::GermanGermany;
    String html="<section class='card'><h2>";
    html+=de ? "SD-Rotationstest" : "SD rotation test";
    html+="</h2><p><strong>";html+=r.passed() ? "PASS" : "FAIL";html+="</strong></p><ul>";
    const char* deLabels[]={"Dateiwechsel","Speicherbudget / älteste Testdateien entfernt","Fremde Testdatei erhalten","Dateiinhalte geprüft","Testdateien aufgeräumt"};
    const char* enLabels[]={"File rotation","Storage budget / oldest test files removed","Unmanaged test file preserved","File contents verified","Test files cleaned up"};
    const bool results[]={r.rotation,r.pruning,r.foreignFile,r.content,r.cleanup};
    for(unsigned i=0;i<5;++i) html+=String("<li>")+(de ? deLabels[i] : enLabels[i])+": "+(results[i] ? "PASS" : "FAIL")+"</li>";
    html+="</ul>";
    if(!r.passed()) {
        html+="<p>";html+=de ? "Details im Systemlog prüfen." : "Check the system log for details.";
        html+="</p>";
        if(!r.cleanup && r.directory[0]) html+=String("<p>")+(de ? "Testreste: " : "Test leftovers: ")+escapeHtml(r.directory)+"</p>";
    }
    html+="<form method='post' action='/sd-card/rotation-test'><button type='submit'>";
    html+=de ? "Erneut testen" : "Repeat test";
    html+="</button></form><a href='/sd-card'>SD</a></section>";return html;
}
}
