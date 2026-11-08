// EnvNode: see repository LICENSE.
#include "TestPage.h"
namespace EnvNode { namespace Testing {
String buildTestPage(Locale locale, bool physicalAvailable) {
    const bool de = locale == Locale::GermanGermany;
    String html("<div id='test-app' data-language='"); html += de ? "de" : "en";
    html += "'><div class='notice'><strong id='test-mode-label'>";
    html += de ? "Messmodus wird geladen" : "Loading measurement mode";
    html += "</strong><p id='test-mode-note'></p></div><section class='card'><label>";
    html += de ? "Betriebsart (Wechsel verwirft Ergebnisse)" : "Mode (changing clears results)";
    html += "<select id='test-mode'><option value='physical'";
    if(!physicalAvailable)html += " disabled";
    html += ">";
    html += de ? "Reale Messung · ursprünglicher Input-Adapter 0.1" : "Physical measurement · original Input-Adapter 0.1";
    html += "</option><option value='simulation'>SIMULATION</option></select></label><label>";
    html += de ? "Zielboard und Revision" : "Target board and revision";
    html += "<select id='test-board'></select></label><button id='test-load'>";
    html += de ? "Board laden / neuer Testlauf" : "Load board / new session";
    html += "</button><button id='test-refresh'>";
    html += de ? "Remote-Profile aktualisieren" : "Refresh remote profiles";
    html += "</button><p id='test-profile-status' role='status'></p><p id='test-reference' class='help'></p><p id='test-directed' class='help'></p><details><summary>";
    html += de ? "Quelle der Prüfdaten" : "Test data source";
    html += "</summary><p id='test-source' class='help'></p></details></section><section class='card'><h2 id='test-instruction'></h2><p id='test-summary' role='status' aria-live='polite'></p><label id='test-fault-wrap'>";
    html += de ? "Simulierter Zustand" : "Simulated condition";
    html += "<select id='test-fault'><option value='none'>OK</option><option value='open'>";
    html += de ? "Kontakt 1 unterbrochen" : "Contact 1 open";
    html += "</option><option value='short'>";
    html += de ? "Kontakt 1 mit Zusatzverbindung" : "Contact 1 with extra connection";
    html += "</option><option value='diode-short'>";
    html += de ? "Gerichtete Sperrpfade überbrückt" : "Directed blocking paths bridged";
    html += "</option></select></label><label id='test-confirm-wrap'><input type='checkbox' id='test-confirm'>";
    html += de ? "Prüfling ohne Versorgung und ohne ESP32; Provisioning-Kabel getrennt, richtiger Pigtail verbunden. Nur ursprünglicher Input-Adapter 0.1." : "Target unpowered, ESP32 removed, provisioning cable disconnected, correct pigtail connected. Original Input-Adapter 0.1 only.";
    html += "</label><div class='actions'><button id='test-start'>";
    html += de ? "Verbunden – Prüfung starten" : "Connected – start test";
    html += "</button><button id='test-next'>";
    html += de ? "Nächster Anschluss" : "Next connector";
    html += "</button><button id='test-abort' class='danger'>";
    html += de ? "Abbrechen" : "Abort";
    html += "</button></div><progress id='test-progress' value='0' max='1' style='width:100%;margin-top:16px'></progress><p id='test-error' class='error' role='alert'></p></section>";
    html += "<div class='test-layout'><section class='card'><div id='test-drawing'></div><p class='help'>";
    html += de ? "KiCad-Ansicht von oben. Runde LEDs: Sockel- und Referenzkontakte. Quadratische LEDs: Anschlusskontakte. Orange = nicht vollständig prüfbar, rot = Fehler, grün = erwarteter Zustand, grau = noch nicht geprüft." : "KiCad top view. Round LEDs: socket and reference contacts. Square LEDs: connector contacts. Amber = incomplete coverage, red = fault, green = expected state, gray = pending.";
    html += "</p></section><section class='card'><h2>";
    html += de ? "Anschlussliste" : "Connector list";
    html += "</h2><p class='help'>";
    html += de ? "Anschluss direkt auswählen oder erneut prüfen. Während einer Prüfung ist die Auswahl gesperrt." : "Select any connector to inspect or retest. Selection is locked while a test is running.";
    html += "</p><ol id='test-connectors'></ol></section></div><section class='card'><h2>";
    html += de ? "Einzelprüfungen: Kontakt gegen Prüfeingang" : "Individual checks: contact against measurement input";
    html += "</h2><p class='help'>";
    html += de ? "● Durchgang erwartet · ○ Trennung erwartet · ! unerwarteter Durchgang · × fehlender Durchgang · E Messfehler · D? gerichteter Pfad, Gegenprüfung fehlt · D gerichteter Pfad bestätigt · S Sperrrichtung getrennt. Kontakte ohne positiven Prüfpfad bleiben unvollständig." : "● Expected continuity · ○ Expected isolation · ! Unexpected continuity · × Missing continuity · E Measurement error · D? directed path awaiting reverse check · D directed path verified · S reverse direction isolated. Contacts without a positive test path remain incomplete.";
    html += "</p><div class='table-scroll'><table id='test-matrix'></table></div></section></div><style>#test-app [hidden]{display:none!important}.test-layout{display:grid;grid-template-columns:minmax(0,2fr) minmax(220px,1fr);gap:16px}.test-layout svg{width:100%;max-height:700px;background:#17483e;border-radius:8px}.test-layout ol{padding-left:24px}.test-layout li{padding:4px}.test-layout li.current{font-weight:750;background:#fff0d7}.test-layout li button{display:block;width:100%;text-align:left;background:transparent;color:var(--ink);padding:5px 3px;white-space:normal}.test-layout li button:hover:not(:disabled){background:#fff0d7}.test-layout li.current button{font-weight:750}#test-matrix td,#test-matrix th{padding:4px;min-width:22px;text-align:center}#test-matrix th:first-child{min-width:160px;text-align:left}button:disabled{opacity:.45;cursor:default}@media(max-width:850px){.test-layout{grid-template-columns:1fr}}</style><script src='/test/ui.js'></script>";
    return html;
}
} }
