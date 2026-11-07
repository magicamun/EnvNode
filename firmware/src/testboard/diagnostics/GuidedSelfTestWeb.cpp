// EnvNode: see repository LICENSE.
#include "HardwareDiagnostics.h"
#include "../TestBoardFirmwareVersion.h"
namespace EnvNode {
namespace {
bool indexValue(const String& text, uint32_t& value) {
    if (!text.length() || text.length()>10) return false;
    value=0;
    for(size_t i=0;i<text.length();++i) {
        if(text[i]<'0'||text[i]>'9'||value>(UINT32_MAX-uint32_t(text[i]-'0'))/10) return false;
        value=value*10+uint32_t(text[i]-'0');
    }
    return true;
}
String outputLabel(size_t index) {
    return String(index<4 ? "J1 · Pin " : index<8 ? "J2 · Pin " : "J3 · Pin ")+
        String(unsigned(index<4 ? index+1 : index<8 ? index-3 : index-7));
}
String inputLabel(size_t index) {
    if(index<38) return String("Input-Adapter J4 · Pin ")+String(unsigned(index+1));
    return index==38 ? "Input-Adapter TP1 · Pin 2 (MB_3V3_SYS)" : "Input-Adapter TP1 · Pin 1 (MB_GND)";
}
}
void HardwareDiagnostics::selfTestState() {
    using Testing::GuidedSelfTest;
    String json; json.reserve(10000);
    json=String("{\"running\":")+(selfTest_.running()?"true":"false")+",\"selected\":"+String(unsigned(selfSelected_));
    json+=String(",\"active\":")+(active()?"true":"false");
    json+=",\"firmware\":\""+String(FirmwareVersion)+"\"";
    json+=",\"generation\":"+String(selfGeneration_)+",\"results\":[";
    for(size_t i=0;i<GuidedSelfTest::Count;++i) {
        if(i)json+=',';
        const auto& r=selfTest_.result(i);
        json+='[';json+=String(unsigned(r.verdict));json+=',';json+=String(r.low);json+=',';
        json+=String(r.high);json+=',';json+=String(r.isolation);json+=']';
    }
    json+="],\"settleMs\":[";
    for(size_t i=0;i<GuidedSelfTest::Count;++i){if(i)json+=',';json+=String(selfTest_.result(i).settleMs);}
    json+="],\"isolationDetails\":[";
    for(size_t i=0;i<GuidedSelfTest::Count;++i) {
        if(i)json+=',';
        const auto& r=selfTest_.result(i);
        if(r.isolationPath==255){json+="null";continue;}
        json+='[';json+=String(unsigned(r.isolationPath));json+=',';
        json+=String(r.isolationMean);json+=',';json+=String(r.isolationMinimum);json+=',';
        json+=String(unsigned(r.isolationAbove));json+=',';json+=String(r.isolationTotalAbove);json+=',';
        json+=String(r.isolationTotalSamples);json+=',';json+=String(unsigned(r.isolationBadWindows));
        for(auto sample:r.isolationSamples){json+=',';json+=String(sample);}
        json+=']';
    }
    json+="]}";
    host_.server().sendHeader("Cache-Control","no-store");
    host_.server().send(200,"application/json",json);
}
void HardwareDiagnostics::selfTestAction() {
    auto& s=host_.server();
    if(s.arg("action")=="stop") { stop();++selfGeneration_;selfTestState();return; }
    uint32_t generation=0,index=0;
    if(!indexValue(s.arg("generation"),generation)||generation!=selfGeneration_) {
        s.send(409,"text/plain","Veraltete Auswahl. Status aktualisieren.");return;
    }
    if(!host_.administrationAvailable()||!available_()||selfTest_.running()) {
        s.send(409,"text/plain","Erst den laufenden Test beenden.");return;
    }
    if(s.arg("action")=="clear") {
        stop();selfTest_.clear();++selfGeneration_;selfTestState();return;
    }
    if(s.arg("action")=="select") {
        if(!indexValue(s.arg("index"),index)||index>=Testing::GuidedSelfTest::Count) {
            s.send(400,"text/plain","Ungültiger Kontakt");return;
        }
        stop();selfSelected_=index;++selfGeneration_;selfTestState();return;
    }
    if(s.arg("action")!="run"||s.arg("confirm")!="yes"||
        !indexValue(s.arg("index"),index)||index!=selfSelected_) {
        s.send(400,"text/plain","Auswahl oder Bestätigung fehlt");return;
    }
    uint32_t settleMs=15;
    if(s.hasArg("settleMs")&&(!indexValue(s.arg("settleMs"),settleMs)||(settleMs!=15&&settleMs!=50&&settleMs!=100))) {
        s.send(400,"text/plain","Einschwingzeit muss 15, 50 oder 100 ms sein.");return;
    }
    stop();
    selfTest_.start(selfSelected_,millis(),uint16_t(settleMs));++selfGeneration_;selfTestState();
}
void HardwareDiagnostics::selfTestPage() {
    using Testing::GuidedSelfTest;
    String html;html.reserve(16000);
    html=R"HTML(<section class='card'><h2>Geführter Hardware-Selbsttest</h2>
<p><strong>Test-Controller 0.1 + ursprünglicher Input-Adapter 0.1.</strong> Die neuen DevKit-Adapter werden hier noch nicht unterstützt. Kein Mainboard, Modul oder Provisioning-Kabel anschließen. Referenzkabel bleibt frei. Versorgung der Multiplexer muss vorhanden sein.</p>
<p>Die Firmware schaltet LOW/HIGH und prüft danach die anderen zugeordneten Kanäle sowie den deaktivierten Pfad. Provisorische ADC-Grenzen: LOW/Trennung ≤ 200; HIGH 1000–3800. Jeder Schritt: gewählte Einschwingzeit und unverändert 8 Messungen. Das ist eine Inbetriebnahmeprüfung, keine kalibrierte Mainboardprüfung.</p>
<p>Vor dem Umstecken Versorgung ausschalten. Nach jedem Lauf werden die Multiplexer abgeschaltet. Ergebnisse werden im Browser für diese Tabsitzung gesammelt, auch wenn der ESP32 zum Umstecken ausgeschaltet wird. JSON-Protokoll vor dem Schließen herunterladen.</p>
<h3 id='chosen'></h3><p id='instruction'></p><p id='isolation-detail' class='help' style='white-space:pre-line'></p>
<label>Einschwingzeit nach jeder Umschaltung <select id='settle'><option value='15'>15 ms · bisher</option><option value='50'>50 ms</option><option value='100'>100 ms</option></select></label>
<label><input id='confirm' type='checkbox'> Nur die angezeigte Brücke ist gesteckt; keine fremde Spannung / kein Prüfling angeschlossen.</label>
<div class='actions'><button id='run' type='button'>Diesen Kontakt prüfen</button><button id='stop' type='button'>Abbrechen / alles abschalten</button><button id='next' type='button'>Nächster Kontakt</button><button id='export' type='button'>Protokoll herunterladen</button><button id='clear' type='button'>Ergebnisse löschen</button></div>
<p id='progress' role='status'></p><p id='error' role='alert'></p>
<div class='table-scroll'><table><thead><tr><th>Kontakt / Auswahl</th><th>Ergebnis</th><th>LOW</th><th>HIGH</th><th>Trennung max.</th><th>Trennungsdetails</th></tr></thead><tbody id='rows'></tbody></table></div>
<p>Output-Gruppe: Input-Adapter und weitere Brücken abziehen, Kontakt direkt gegen Controller J8/9 (COM_IN). Input-Gruppe: Input-Adapter anschließen, nur Controller J3/2 mit dem gewählten Adapterkontakt verbinden. TP1 benennt die späteren Mainboard-Referenzkontakte und darf hier an keinem Mainboard hängen.</p>
<p><a href='/diagnostics'>Manuelle Hardwarediagnose</a></p></section><script>(()=>{const cases=[)HTML";
    for(size_t i=0;i<GuidedSelfTest::Count;++i) {
        if(i)html+=',';
        const bool in=i>=GuidedSelfTest::OutputCount;
        String label=in?inputLabel(i-GuidedSelfTest::OutputCount):outputLabel(i);
        String instruction=in?String("Controller J3 · Pin 2 ↔ ")+label:
            String("Controller ")+label+" ↔ Controller J8 · Pin 9 (COM_IN). Input-Adapter abziehen.";
        const auto source=GuidedSelfTest::output(in?9:i);
        const auto input=in?Testing::fixtureRevision01().routing.socket[i-GuidedSelfTest::OutputCount]:Testing::MuxAddress{0,0};
        html+="{sourceDevice:"+String(unsigned(source.device))+",sourceChannel:"+String(unsigned(source.channel))+
            ",inputDevice:"+String(in?int(input.device):-1)+",inputChannel:"+String(unsigned(input.channel))+",label:'"+label+"',instruction:'"+instruction+"'}";
    }
    html+=R"HTML(];let state=null,busy=false;const storageKey='envnode-guided-controller01-input01-v2';let saved=Array(cases.length).fill(null);try{const previous=JSON.parse(sessionStorage.getItem(storageKey)||sessionStorage.getItem('envnode-guided-controller01-input01-v1'));if(Array.isArray(previous)&&previous.length===cases.length)saved=previous;}catch(e){}const $=id=>document.getElementById(id),names=['Offen','Läuft','Bestanden','Fehler','Abgebrochen'];
const rows=cases.map((c,i)=>{const tr=document.createElement('tr'),cell=document.createElement('td'),b=document.createElement('button');b.type='button';b.textContent=(i<22?'Output · ':'Input · ')+c.label;b.onclick=()=>command('select',{index:i});cell.append(b);tr.append(cell);for(let j=0;j<5;j++)tr.append(document.createElement('td'));$('rows').append(tr);return {tr,b};});
function values(s,i){const r=s.results[i];return r[0]===0&&saved[i]?saved[i].values:r;}
function timing(s,i){return s.results[i][0]===0&&saved[i]?(saved[i].settleMs||15):((s.settleMs||[])[i]||15);}
function detail(s,i){return s.results[i][0]===0&&saved[i]?(saved[i].detail||null):((s.isolationDetails||[])[i]||null);}
function pathLabel(i,d){if(d[0]===0)return i<22?'Beide Output-Multiplexer aus':'Alle Input-Multiplexer aus';const c=cases[(i<22?0:22)+d[0]-1];return i<22?c.label+' · U'+(c.sourceDevice+1)+'/I'+c.sourceChannel:c.label+' · U'+(c.inputDevice+4)+'/I'+c.inputChannel;}
function summary(s,i,full=false){const d=detail(s,i);if(!d)return s.results[i][0]===0&&!saved[i]?'—':'Keine Detaildaten – Kontakt erneut prüfen.';const text=pathLabel(i,d)+' · Wartezeit '+timing(s,i)+' ms\nMittel '+d[1]+' · Minimum '+d[2]+' · über 200: '+d[3]+'/8';if(!full)return text;const c=cases[i],other=d[0]?cases[(i<22?0:22)+d[0]-1]:null;const out=i>=22?'U'+(c.sourceDevice+1)+'/I'+c.sourceChannel:other?'U'+(other.sourceDevice+1)+'/I'+other.sourceChannel:'Beide aus';const input=i<22||!other?'Alle aus':'U'+(other.inputDevice+4)+'/I'+other.inputChannel;return 'Fenster mit höchstem Einzelwert: '+text+'\n8 Einzelwerte: '+d.slice(7).join(', ')+'\nAlle Trennungsfenster: '+d[4]+'/'+d[5]+' Werte über 200 in '+d[6]+' Fenstern.'+'\nZum Nachmessen: Brücke unverändert lassen. Manuelle Diagnose: Output '+out+', HIGH; Input '+input+'. Danach aktiv schalten und COM_IN gegen GND messen.';}
function render(s){state=s;let changed=false;s.results.forEach((r,i)=>{const collected=r[0]===1?[4,r[1],r[2],r[3]]:r;const diagnostic=(s.isolationDetails||[])[i]||null,settle=(s.settleMs||[])[i]||15;if(r[0]>=1&&(!saved[i]||JSON.stringify(saved[i].values)!==JSON.stringify(collected)||JSON.stringify(saved[i].detail||null)!==JSON.stringify(diagnostic)||(saved[i].settleMs||15)!==settle)){saved[i]={values:collected,detail:diagnostic,settleMs:settle,at:new Date().toISOString()};changed=true;}});if(changed)try{sessionStorage.setItem(storageKey,JSON.stringify(saved));}catch(e){$('error').textContent='Browser-Sitzungsspeicher nicht verfügbar. Protokoll vor Neustart herunterladen.';}const c=cases[s.selected];$('chosen').textContent=c.label;$('instruction').textContent=c.instruction;$('isolation-detail').textContent=summary(s,s.selected,true);$('run').disabled=busy||s.running;$('settle').disabled=busy||s.running;$('next').disabled=busy||s.running||s.selected===cases.length-1;$('export').disabled=busy||s.running;$('clear').disabled=busy||s.running;
let pass=0,fail=0;rows.forEach((r,i)=>{const v=values(s,i);r.b.disabled=busy||s.running;r.tr.style.background=i===s.selected?'#fff0d7':'';r.tr.cells[1].textContent=names[v[0]];r.tr.cells[2].textContent=v[0]===0?'—':v[1];r.tr.cells[3].textContent=v[0]===0?'—':v[2];r.tr.cells[4].textContent=v[0]===0?'—':v[3];r.tr.cells[5].textContent=summary(s,i);r.tr.cells[5].style.whiteSpace='pre-line';if(v[0]===2)pass++;if(v[0]===3)fail++;});$('progress').textContent=(s.running?'Messung läuft … ':s.active?'Manuelle Diagnose aktiv. ':'Ausgänge abgeschaltet. ')+pass+' bestanden · '+fail+' fehlerhaft · '+cases.length+' Kontakte insgesamt';}
async function command(action,extra={}){if(busy||!state)return;if(action==='run'&&!$('confirm').checked){$('error').textContent='Bitte die Verbindung bestätigen.';return;}busy=true;render(state);try{const p=new URLSearchParams({action,settleMs:$('settle').value,generation:state.generation,index:state.selected,confirm:$('confirm').checked?'yes':'',...extra});const r=await fetch('/selftest/action',{method:'POST',body:p});if(!r.ok)throw Error(await r.text());render(await r.json());$('error').textContent='';if(action==='select')$('confirm').checked=false;if(action==='clear'){saved=Array(cases.length).fill(null);try{sessionStorage.removeItem(storageKey);sessionStorage.removeItem('envnode-guided-controller01-input01-v1');}catch(e){}render(state);}}catch(e){$('error').textContent=e.message;}finally{busy=false;if(state)render(state);}}
$('clear').onclick=()=>{if(confirm('Alle gesammelten Ergebnisse löschen?'))command('clear');};
$('run').onclick=()=>command('run');$('stop').onclick=()=>command('stop');$('next').onclick=()=>command('select',{index:state.selected+1});
$('export').onclick=()=>{if(!state||state.running)return;const report={schema:2,fixture:'controller0.1-input0.1-module0.1',firmware:state.firmware,mode:'physical-guided-loopback',thresholds:{lowMaximum:200,highMinimum:1000,highMaximum:3800},capturedAtBrowser:new Date().toISOString(),results:cases.map((c,i)=>({contact:c.label,instruction:c.instruction,verdict:names[values(state,i)[0]],low:values(state,i)[1],high:values(state,i)[2],isolationMaximum:values(state,i)[3],settleMs:timing(state,i),isolationDetails:detail(state,i)?{path:pathLabel(i,detail(state,i)),mean:detail(state,i)[1],minimum:detail(state,i)[2],aboveLimit:detail(state,i)[3],sampleCount:8,totalAboveLimit:detail(state,i)[4],totalSamples:detail(state,i)[5],badWindows:detail(state,i)[6],samples:detail(state,i).slice(7)}:null,testedAtBrowser:saved[i]?saved[i].at:null}))};const url=URL.createObjectURL(new Blob([JSON.stringify(report,null,2)],{type:'application/json'})),a=document.createElement('a');a.href=url;a.download='envnode-hardware-selbsttest.json';a.click();setTimeout(()=>URL.revokeObjectURL(url),1000);};
async function poll(){try{if(!busy){const r=await fetch('/selftest/state',{cache:'no-store'});if(!r.ok)throw Error('Status nicht erreichbar');render(await r.json());}}catch(e){$('error').textContent=e.message;}setTimeout(poll,300);}poll();
addEventListener('pagehide',()=>navigator.sendBeacon('/selftest/action',new URLSearchParams({action:'stop'})));})();</script>)HTML";
    pages_.sendPage("Hardware-Selbsttest","/selftest",std::move(html),200,true);
}
}
