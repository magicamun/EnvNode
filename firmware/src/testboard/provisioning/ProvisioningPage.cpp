// EnvNode: see repository LICENSE.
#include "ProvisioningPage.h"
#include "HtmlEscaping.h"
#include "JsonWriter.h"
#include "BoardIdentityProfiles.h"
#include "testing/TestModel.h"
#include <cstring>
namespace EnvNode {
namespace {
String boardImages() {
    String json="{";bool comma=false;
    for(size_t i=0;i<boardIdentityProfileCount();++i) {
        const auto& p=*boardIdentityProfileAt(i);
        const char* prefix=p.id==BoardProfileId::EnvNodeMini?"mini-":p.id==BoardProfileId::EnvNodeWeather?"weather-":nullptr;
        if(!prefix)continue;
        const String rev=String(p.revision.major)+"."+String(p.revision.minor);
        for(size_t j=0;j<Testing::boardCount();++j) {
            const auto& b=*Testing::boardAt(j);
            if(strncmp(b.id,prefix,strlen(prefix)) || rev!=b.identityRevision)continue;
            if(comma)json+=',';comma=true;
            appendJsonString(json,(String(encodeBoardProfileId(p.id))+":"+rev).c_str());json+=':';
            appendJsonString(json,b.id);break;
        }
    }
    json+='}';return json;
}
String boardImageSection(bool german) {
    String c="<section class='card'><h2>";
    c+=german?"Boardansicht":"Board view";
    c+="</h2><figure style='margin:0'><img id='provision-board-image' hidden style='max-width:100%;max-height:520px;background:#17483e;border-radius:8px' alt=''><figcaption id='provision-board-caption'></figcaption></figure></section><script>";
    c+="document.addEventListener('DOMContentLoaded',()=>{const views="+boardImages()+";";
    c+=german?"const de=true;":"const de=false;";
    c+=R"JS(
const select=document.querySelector('[name=boardProfileKey]'),image=document.getElementById('provision-board-image'),caption=document.getElementById('provision-board-caption');
if(!select)return;
let generation=0;
const update=async()=>{
 const current=++generation,id=views[select.value];image.hidden=true;
 caption.textContent=id?(de?'Boardansicht wird geladen…':'Loading board view…'):(de?'Für diese Auswahl ist keine Boardansicht verfügbar.':'No board view available for this selection.');
 if(!id)return;
 const fallback='/test/board.svg?board='+encodeURIComponent(id);
 let src=fallback,label=id;
 try{const response=await fetch('/test/definition?board='+encodeURIComponent(id),{cache:'no-store'});if(response.ok){const board=await response.json();src=board.imageUrl||fallback;label=board.name+' · '+board.identityRevision+' · PCB '+board.pcbRevision;}}catch(e){}
 if(current!==generation)return;
 image.onerror=()=>{image.onerror=()=>{image.hidden=true;caption.textContent=de?'Board-Bild nicht verfügbar.':'Board image unavailable.';};image.src=fallback;};
 image.alt=label;caption.textContent=label;image.src=src;image.hidden=false;
};
select.addEventListener('change',update);update();
});
)JS";
    return c+"</script>";
}
}
String buildProvisioningOverview(TestBoardProvisioning& provisioning, Locale locale) {
    const bool german = locale == Locale::GermanGermany;
    const auto current = provisioning.refresh();
    String c = "<section class='card'><h2>Provisioning</h2><p>";
    c += german
        ? "Reales EEPROM des Prüflings an I²C: SDA GPIO16, SCL GPIO5, Adresse 0x50. ESP32 des Prüflings entfernen, andere Versorgungen sowie Mess-Pigtails und Input-Adapter trennen. Erst danach den separaten Provisioning-Stecker anschließen."
        : "Real target EEPROM on I²C: SDA GPIO16, SCL GPIO5, address 0x50. Remove the target ESP32 and disconnect other supplies, measurement pigtails and Input Adapter before connecting the separate provisioning cable.";
    c += "</p><p>";
    c += german ? "Nach dem Provisioning das Kabel vor der Durchgangsprüfung wieder abziehen. Die Firmware schaltet die Versorgung nicht ab." : "Disconnect the cable again before continuity testing. Firmware does not switch off its supply.";
    c += "</p><p>";
    c += german ? "EEPROM-Status: " : "EEPROM status: ";
    // Status strings are fixed diagnostics, not device-provided HTML.
    c += boardIdentityStatusName(current.status);
    if (current.status == BoardIdentityStatus::Valid || current.status == BoardIdentityStatus::UnassignedSerial) {
        c += german ? " · Gelesene Identität: " : " · Read identity: ";
        ProvisioningBoardProfile profile;
        if (provisioning.provisioningProfile(current.identity.profileId, profile)) c += escapeHtml(profile.displayName);
        c += " " + String(current.identity.revision.major) + "." + String(current.identity.revision.minor);
        c += " · " + escapeHtml(current.identity.serialNumber.value);
    }
    c += "</p><p>";
    c += german ? "Unterstützt: Mini 0.8 und 0.9, Weather 0.2 und 0.3 sowie Mainboard 0.3. Die tatsächliche Hardware-Revision auswählen." : "Supported: Mini 0.8 and 0.9, Weather 0.2 and 0.3, and Mainboard 0.3. Select the actual hardware revision.";
    return c + "</p></section>" + boardImageSection(german);
}
}
