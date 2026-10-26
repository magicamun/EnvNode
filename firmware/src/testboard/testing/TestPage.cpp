// EnvNode: see repository LICENSE.
#include "TestPage.h"
namespace EnvNode { namespace Testing {
String buildTestPage(Locale locale) {
    const bool de = locale == Locale::GermanGermany;
    String html("<div id='test-app' data-language='"); html += de ? "de" : "en";
    html += "'><div class='notice'><strong>SIMULATION</strong> · ";
    html += de ? "Keine reale Messung. Prüfboard-Ausgänge bleiben inaktiv." : "No physical measurement. Fixture outputs remain inactive.";
    html += "</div><section class='card'><label>";
    html += de ? "Zielboard und Revision" : "Target board and revision";
    html += "<select id='test-board'></select></label><button id='test-load'>";
    html += de ? "Board laden / neuer Testlauf" : "Load board / new session";
    html += "</button><details><summary>";
    html += de ? "Quelle der Prüfdaten" : "Test data source";
    html += "</summary><p id='test-source' class='help'></p></details></section><section class='card'><h2 id='test-instruction'></h2><p id='test-summary' role='status' aria-live='polite'></p><label>";
    html += de ? "Simulierter Zustand" : "Simulated condition";
    html += "<select id='test-fault'><option value='none'>OK</option><option value='open'>";
    html += de ? "Kontakt 1 unterbrochen" : "Contact 1 open";
    html += "</option><option value='short'>";
    html += de ? "Kontakt 1 mit Zusatzverbindung" : "Contact 1 with extra connection";
    html += "</option></select></label><div class='actions'><button id='test-start'>";
    html += de ? "Verbunden – Prüfung starten" : "Connected – start test";
    html += "</button><button id='test-next'>";
    html += de ? "Nächster Anschluss" : "Next connector";
    html += "</button><button id='test-all'>Check all (Simulation)</button><button id='test-abort' class='danger'>";
    html += de ? "Abbrechen" : "Abort";
    html += "</button></div><progress id='test-progress' value='0' max='1' style='width:100%;margin-top:16px'></progress><p id='test-error' class='error' role='alert'></p></section>";
    html += "<div class='test-layout'><section class='card'><div id='test-drawing'></div><p class='help'>";
    html += de ? "KiCad-Ansicht von oben. Runde LEDs: Sockelkontakte. Quadratische LEDs: Anschlusskontakte. Orange = nicht vollständig prüfbar, rot = Fehler, grün = erwarteter Zustand, grau = noch nicht geprüft." : "KiCad top view. Round LEDs: socket contacts. Square LEDs: connector contacts. Amber = incomplete coverage, red = fault, green = expected state, gray = pending.";
    html += "</p></section><section class='card'><h2>";
    html += de ? "Anschlussliste" : "Connector list";
    html += "</h2><p class='help'>";
    html += de ? "Anschluss direkt auswählen oder erneut prüfen. Während einer Prüfung ist die Auswahl gesperrt." : "Select any connector to inspect or retest. Selection is locked while a test is running.";
    html += "</p><ol id='test-connectors'></ol></section></div><section class='card'><h2>";
    html += de ? "Einzelprüfungen: Kontakt gegen Sockelpin 1–38" : "Individual checks: contact against socket pin 1–38";
    html += "</h2><p class='help'>";
    html += de ? "● Durchgang erwartet · ○ Trennung erwartet · ! unerwarteter Durchgang · × fehlender Durchgang · E Messfehler. Kontakte ohne positiven Prüfpfad bleiben unvollständig." : "● Expected continuity · ○ Expected isolation · ! Unexpected continuity · × Missing continuity · E Measurement error. Contacts without a positive test path remain incomplete.";
    html += "</p><div class='table-scroll'><table id='test-matrix'></table></div></section></div><style>.test-layout{display:grid;grid-template-columns:minmax(0,2fr) minmax(220px,1fr);gap:16px}.test-layout svg{width:100%;max-height:700px;background:#17483e;border-radius:8px}.test-layout ol{padding-left:24px}.test-layout li{padding:4px}.test-layout li.current{font-weight:750;background:#fff0d7}.test-layout li button{display:block;width:100%;text-align:left;background:transparent;color:var(--ink);padding:5px 3px;white-space:normal}.test-layout li button:hover:not(:disabled){background:#fff0d7}.test-layout li.current button{font-weight:750}#test-matrix td,#test-matrix th{padding:4px;min-width:22px;text-align:center}#test-matrix th:first-child{min-width:160px;text-align:left}button:disabled{opacity:.45;cursor:default}@media(max-width:850px){.test-layout{grid-template-columns:1fr}}</style><script src='/test/ui.js'></script>";
    return html;
}
} }
