// EnvNode: see repository LICENSE.
#include "HardwareDiagnostics.h"
#include <Arduino.h>
namespace EnvNode {
namespace {
bool number(WebServer& server, const char* key, int low, int high, int& value) {
    String s = server.arg(key);
    if (!s.length()) return false;
    int n = 0; size_t i = 0; bool negative = s[0] == '-';
    if (negative) ++i;
    if (i == s.length()) return false;
    for (; i < s.length(); ++i) {
        if (s[i] < '0' || s[i] > '9' || n > 100) return false;
        n = n * 10 + s[i] - '0';
    }
    if (negative) n = -n;
    if (n < low || n > high) return false;
    value = n; return true;
}
}
void HardwareDiagnostics::stop() {
    if (!active_) return;
    const auto& f = Testing::fixtureRevision01();
    for (auto pin : f.outputEnable) digitalWrite(pin, HIGH);
    for (auto pin : f.inputEnable) digitalWrite(pin, HIGH);
    pinMode(f.drive, INPUT);
    active_ = false;
}
void HardwareDiagnostics::apply() {
    const auto& f = Testing::fixtureRevision01();
    // Disable every path before changing address or drive level.
    for (auto pin : f.outputEnable) { digitalWrite(pin, HIGH); pinMode(pin, OUTPUT); }
    for (auto pin : f.inputEnable) { digitalWrite(pin, HIGH); pinMode(pin, OUTPUT); }
    pinMode(f.drive, INPUT);
    for (int i = 0; i < 4; ++i) {
        digitalWrite(f.outputSelect[i], (channel_ >> i) & 1); pinMode(f.outputSelect[i], OUTPUT);
        digitalWrite(f.inputSelect[i], (inputChannel_ >> i) & 1); pinMode(f.inputSelect[i], OUTPUT);
    }
    pinMode(f.sense, INPUT);
    analogReadResolution(12);
    analogSetPinAttenuation(f.sense, ADC_11db);
    if (output_ >= 0) {
        digitalWrite(f.drive, level_); pinMode(f.drive, OUTPUT);
        digitalWrite(f.outputEnable[output_], LOW);
    }
    if (input_ >= 0) digitalWrite(f.inputEnable[input_], LOW);
    lastCommand_ = millis(); active_ = true;
}
void HardwareDiagnostics::tick(uint32_t now) {
    if (active_ && (!host_.administrationAvailable() || uint32_t(now - lastCommand_) >= 60000)) stop();
}
void HardwareDiagnostics::state() {
    String json = String("{\"active\":") + (active_ ? "true" : "false") + ",\"remaining\":";
    json += active_ ? String((60000 - uint32_t(millis() - lastCommand_)) / 1000) : "0";
    json += ",\"adc\":";
    json += active_ ? String(analogRead(Testing::fixtureRevision01().sense)) : "null";
    json += "}";
    host_.server().sendHeader("Cache-Control", "no-store");
    host_.server().send(200, "application/json", json);
}
void HardwareDiagnostics::registerRoutes(WebServer& server) {
    server.on("/diagnostics", HTTP_GET, [this]() { page(); });
    server.on("/diagnostics/state", HTTP_GET, [this]() { tick(millis()); state(); });
    server.on("/diagnostics/action", HTTP_POST, [this]() {
        auto& s = host_.server();
        if (s.arg("action") == "stop") { stop(); state(); return; }
        if (!host_.administrationAvailable() || !available_()) { s.send(409,"text/plain","Diagnose derzeit nicht verfügbar"); return; }
        int out, channel, level, in, inChannel;
        if (s.arg("action") != "apply" || s.arg("confirm") != "yes" ||
            !number(s,"output",-1,1,out) || !number(s,"channel",0,15,channel) ||
            !number(s,"level",0,1,level) || !number(s,"input",-1,2,in) ||
            !number(s,"inputChannel",0,15,inChannel)) {
            s.send(400,"text/plain","Ungültige Auswahl oder Freigabe fehlt"); return;
        }
        output_=out; channel_=channel; level_=level; input_=in; inputChannel_=inChannel;
        apply(); state();
    });
}
void HardwareDiagnostics::page() {
    String html = R"HTML(<section class='card'><h2>Hardwarediagnose · Controller 0.1</h2>
<p>Reale GPIO-Ansteuerung. Ohne Prüfling beginnen. Multiplexer benötigen 3,3 V an 3V3_SYS und gemeinsame Masse. GPIO34 darf höchstens 3,3 V erhalten.</p>
<p>Kanäle sind die Hardwareadressen I0–I15, unabhängig vom Boardprofil. Automatische Abschaltung 60 Sekunden nach dem letzten Schaltbefehl.</p>
<form id='diag'><label>Output-Multiplexer <select name='output'><option value='-1'>Beide aus</option><option value='0'>U1 · GPIO / I²C</option><option value='1'>U2 · Modul</option></select></label>
<label>Output-Kanal <input name='channel' type='number' min='0' max='15' value='0' required></label>
<label>Ausgang GPIO17 <select name='level'><option value='0'>LOW</option><option value='1'>HIGH · 3,3 V</option></select></label>
<label>Input-Multiplexer <select name='input'><option value='-1'>Alle aus · ohne Adapter</option><option value='0'>U4 / EN1</option><option value='1'>U5 / EN2</option><option value='2'>U6 / EN3</option></select></label>
<label>Input-Kanal <input name='inputChannel' type='number' min='0' max='15' value='0' required></label>
<label><input type='checkbox' name='confirm' value='yes' required> Test-Controller ist angeschlossen; reale Ausgänge freigeben</label>
<button type='submit'>Schalten / 60 Sekunden freigeben</button></form>
<button id='stop' type='button'>Alle Multiplexer abschalten</button>
<p id='reading'>Status wird geladen …</p><p id='error' role='alert'></p>
<p>GPIO34: ADC-Rohwert 0–4095, keine kalibrierte Spannung. Ohne Verbindung zieht R9 den Eingang gegen GND.</p>
<p>Schleifentest: J1 Pin 1 entspricht U1/I8. Für HIGH/LOW diesen Kontakt mit J8 Pin 9 (COM_IN) verbinden. Der Ausgang enthält bereits R6 = 1 kΩ; R9 = 2,2 kΩ bildet einen Spannungsteiler, daher ist bei HIGH kein ADC-Vollausschlag zu erwarten.</p>
</section><script>(()=>{const form=document.getElementById('diag'),err=document.getElementById('error'),reading=document.getElementById('reading');
function show(s){reading.textContent=(s.active?'Aktiv · noch '+s.remaining+' s · GPIO34 ADC: '+s.adc:'Inaktiv · Multiplexer deaktiviert');}
async function command(data){try{const r=await fetch('/diagnostics/action',{method:'POST',body:data});if(!r.ok)throw Error(await r.text());show(await r.json());err.textContent='';}catch(e){err.textContent=e.message;}}
form.onsubmit=e=>{e.preventDefault();const p=new URLSearchParams(new FormData(form));p.set('action','apply');command(p);};
document.getElementById('stop').onclick=()=>command(new URLSearchParams({action:'stop'}));
async function poll(){try{const r=await fetch('/diagnostics/state',{cache:'no-store'});if(!r.ok)throw Error('Status nicht erreichbar');show(await r.json());}catch(e){err.textContent=e.message;}setTimeout(poll,1000);}poll();
addEventListener('pagehide',()=>navigator.sendBeacon('/diagnostics/action',new URLSearchParams({action:'stop'})));})();</script>)HTML";
    pages_.sendPage("Hardwarediagnose", "/diagnostics", html);
}
}
