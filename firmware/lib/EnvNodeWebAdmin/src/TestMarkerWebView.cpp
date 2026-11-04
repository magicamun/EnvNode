// EnvNode: see repository LICENSE.
#include "TestMarkerWebView.h"
#include "HtmlEscaping.h"
#include <cstring>
namespace EnvNode {
bool matchingTestMarkerBoot(const String& text,const char* current) {
    return current && strlen(current)==32 && text.length()==32 && !memcmp(text.c_str(),current,32);
}
bool parseTestMarkerId(const String& text,uint32_t& value) {
    value=0;if(text.isEmpty()||text.length()>10)return false;
    for(size_t i=0;i<text.length();++i) {
        if(text[i]<'0'||text[i]>'9')return false;
        const uint32_t digit=text[i]-'0';if(value>(UINT32_MAX-digit)/10)return false;
        value=value*10+digit;
    }
    return true;
}
int testMarkerHttpStatus(TestMarkerResult result) {
    switch(result) {
        case TestMarkerResult::Accepted:return 303;
        case TestMarkerResult::Invalid:return 400;
        case TestMarkerResult::Conflict:case TestMarkerResult::Exhausted:return 409;
        default:return 503;
    }
}
const char* testMarkerResultText(TestMarkerResult result,Locale locale) {
    const bool de=locale==Locale::GermanGermany;
    switch(result) {
        case TestMarkerResult::Accepted:return de?"Marker in RAM-Warteschlange aufgenommen. UDP-Empfang ist nicht bestätigt.":"Marker accepted into RAM queue. UDP receipt is not confirmed.";
        case TestMarkerResult::Invalid:return de?"Testname und Notiz müssen gültiger UTF-8-Text mit jeweils höchstens 32 Bytes sein. Beginn benötigt einen Testnamen; eine einzelne Notiz benötigt Text.":"Test name and note must be valid UTF-8, at most 32 bytes each. Begin requires a test name; a standalone note requires text.";
        case TestMarkerResult::Conflict:return de?"Der aktive Test hat sich geändert oder diese Aktion passt nicht zum Teststatus. Seite neu laden; es wurde kein Marker gesetzt.":"The active test changed or this action does not match its state. Reload the page; no marker was recorded.";
        case TestMarkerResult::Exhausted:return de?"Keine weitere Test-ID in diesem Boot verfügbar.":"No further test ID available in this boot.";
        case TestMarkerResult::Disabled:return de?"Netzwerklogging ist nicht aktiv. Teststatus unverändert.":"Network logging is disabled. Test state unchanged.";
        case TestMarkerResult::Full:return de?"Diagnosepuffer voll. Marker nicht angenommen; Teststatus unverändert. Bitte erneut versuchen.":"Diagnostic buffer full. Marker not accepted; test state unchanged. Please retry.";
        case TestMarkerResult::Busy:return de?"Diagnosepuffer belegt. Marker nicht angenommen; bitte erneut versuchen.":"Diagnostic buffer busy. Marker not accepted; please retry.";
        default:return de?"Netzwerklogging, WLAN oder Diagnosepuffer nicht verfügbar. Teststatus unverändert.":"Network logging, WiFi or diagnostic buffer unavailable. Test state unchanged.";
    }
}
String buildTestMarkerHtml(const TestMarkerSession& session,Locale locale,bool available,const char* bootId) {
    const bool de=locale==Locale::GermanGermany;
    String html="<section class='card' id='test-markers'><h2>";
    html+=de?"Testmarker":"Test markers";html+="</h2><p>";
    html+=de?"Beginn, Notizen und Ende im strukturierten Diagnoselog markieren. Die Diagnose-Checkboxen bleiben unverändert; die Detaildiagnose des Sensors/Controllers separat auswählen.":"Mark begin, notes and end in the structured diagnostic log. Diagnostic checkboxes remain unchanged; select sensor/controller detail logging separately.";
    html+="</p><p>";
    if(session.active()) {html+=de?"Aktiver Test #":"Active test #";html+=String(session.activeId());html+=" · ";html+=escapeHtml(session.label());}
    else html+=de?"Kein aktiver Test. Eine Notiz kann auch einzeln gesetzt werden.":"No active test. Notes may also be recorded on their own.";
    html+="</p><form method='post' action='/network-logging/marker'><input type='hidden' name='markerBoot' value='";
    html+=escapeHtml(bootId?bootId:"");html+="'><input type='hidden' name='markerId' value='";
    html+=String(session.activeId());html+="'><label>";html+=de?"Testname":"Test name";
    html+="<input name='markerLabel' maxlength='32' value='";html+=escapeHtml(session.label());html+="'";
    if(session.active())html+=" readonly";html+="></label><label>";html+=de?"Notiz (optional)":"Note (optional)";
    html+="<input name='markerNote' maxlength='32'></label><p class='help'>";
    html+=de?"Je Feld höchstens 32 UTF-8-Bytes (Umlaute zählen mehrfach). Marker gelten nur für diesen Boot; UDP kann Daten verlieren.":"At most 32 UTF-8 bytes per field (non-ASCII characters use multiple bytes). Markers belong to this boot; UDP may lose records.";
    html+="</p><div class='actions'>";
    const auto button=[&](const char* phase,const char* text,bool enabled) {
        html+="<button type='submit' name='phase' value='";html+=phase;html+="'";
        if(!enabled)html+=" disabled";html+=">";html+=text;html+="</button>";
    };
    button("begin",de?"Beginn markieren":"Mark begin",available&&!session.active());
    button("note",de?"Notiz setzen":"Add note",available);
    button("end",de?"Ende markieren":"Mark end",available&&session.active());
    html+="</div></form>";
    if(!available)html+=de?"<p class='help'>Netzwerklogging und WLAN müssen aktiv sein.</p>":"<p class='help'>Network logging and WiFi must be enabled.</p>";
    html+="</section>";return html;
}
}
