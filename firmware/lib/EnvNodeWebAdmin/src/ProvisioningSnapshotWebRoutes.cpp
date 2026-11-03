// EnvNode: see repository LICENSE.
#include "HardwareProvisioningWebRoutes.h"
#include "JsonWriter.h"
#include <cstring>
namespace EnvNode {
void HardwareProvisioningWebRoutes::handleEepromSnapshot() {
    if(!operationAllowed())return;
    const String kind=server_.arg("kind"),slot=server_.arg("slot");
    if((kind!="board"&&kind!="module")||(kind=="module"&&(!options_.modules||(slot!="A"&&slot!="B")))){server_.send(400,"application/json","{\"error\":\"InvalidTarget\"}");return;}
    ProvisioningSnapshot snapshot;
    administration_.readProvisioningSnapshot(kind=="module",slot=="B"?ModuleSlot::B:ModuleSlot::A,snapshot);
    String body="{";
    auto field=[&](const char* key,const char* value){if(body.length()>1)body+=',';appendJsonString(body,key);body+=':';appendJsonString(body,value);};
    field("status",provisioningSnapshotStatusName(snapshot.status));field("token",snapshot.token);field("name",snapshot.name);
    field("revision",(String(snapshot.revision.major)+"."+String(snapshot.revision.minor)).c_str());
    field("serial",snapshot.serial);field("batch",snapshot.batch);field("date",snapshot.date);body+='}';
    server_.sendHeader("Cache-Control","no-store");server_.send(200,"application/json",body);
}
bool HardwareProvisioningWebRoutes::verifySnapshot(bool module,ModuleSlot slot) {
    // Older integrations retain their explicit-confirmation POST contract. Every
    // shared-dialog form carries snapshotToken, including its initially empty value.
    if(!server_.hasArg("snapshotToken"))return true;
    ProvisioningSnapshot current;
    const bool available=administration_.readProvisioningSnapshot(module,slot,current);
    const String token=server_.arg("snapshotToken");
    if(available&&current.status!=ProvisioningSnapshotStatus::Unavailable&&current.token[0]&&token==current.token)return true;
    const bool de=pages_.webLocale()==Locale::GermanGermany;
    sendPage("Provisioning","/hardware",String("<section class='card'><p>")+
        (de?"EEPROM nicht erreichbar oder seit der Anzeige verändert. Erneut lesen, Angaben prüfen und neu bestätigen. Es wurde nichts geschrieben.":"EEPROM unavailable or changed since displayed. Read again, review and confirm again. Nothing was written.")+
        "</p><a href='/hardware'>Provisioning</a></section>",409);
    return false;
}
}
