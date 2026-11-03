// EnvNode: see repository LICENSE.
#include "DiagnosticWebView.h"
namespace EnvNode {
bool extendedDiagnosticsChoice(bool previous,bool available,bool editable,bool submitted) {return available&&editable?submitted:previous;}
String renderExtendedDiagnosticsCheckbox(bool enabled,bool available,Locale locale) {
    const bool de=locale==Locale::GermanGermany;
    String html="<label class='choice'><input type='checkbox' name='extendedDiagnostics' value='1'";
    if(enabled)html+=" checked";if(!available)html+=" disabled";
    html+=">";html+=de?"Erweiterte Diagnose":"Extended diagnostics";html+="</label>";
    if(!available)html+=de?"<p class='help'>Nur mit eingesteckter SD-Karte verfügbar. Die gespeicherte Auswahl bleibt erhalten.</p>":"<p class='help'>Available with an inserted SD card. The saved selection is retained.</p>";
    html+="<input type='hidden' name='extendedDiagnosticsEditable' value='";html+=available?"1":"0";html+="'>";
    return html;
}
}
