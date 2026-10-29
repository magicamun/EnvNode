// EnvNode: see repository LICENSE.
#include "HardwareProvisioningWebRoutes.h"
#include "ModuleProfileTemplate.h"
#include "InstanceUuid.h"
#include "HtmlEscaping.h"
#include <cstring>
#include <new>
namespace EnvNode {
namespace {
#include "BrowserHardwareProfiles.generated.inc"
#include "ProvisioningDialog.generated.inc"
#include "ProvisioningModule.generated.inc"
int hexDigit(char c) {
    if(c>='0'&&c<='9')return c-'0';
    if(c>='a'&&c<='f')return c-'a'+10;
    return -1;
}
bool uploadedHex(const String& input,size_t maximum,std::unique_ptr<uint8_t[]>& bytes,size_t& size) {
    size=0;
    if(!input.length()||input.length()%2||input.length()>maximum*2)return false;
    const size_t count=input.length()/2;
    bytes.reset(new(std::nothrow) uint8_t[count]);if(!bytes)return false;
    for(size_t i=0;i<count;++i){const int a=hexDigit(input[2*i]),b=hexDigit(input[2*i+1]);
        if(a<0||b<0)return false;bytes[i]=(a<<4)|b;}
    size=count;return true;
}
}
bool HardwareProvisioningWebRoutes::enableRemoteProfiles() {
    remoteProfilesEnabled_=true;
    return true;
}
void HardwareProvisioningWebRoutes::handleProfileState(){
    if(!operationAllowed())return;
    server_.send(200,"application/json","{\"available\":true}");
}
String HardwareProvisioningWebRoutes::remoteProfilesSection(){
    const bool de=pages_.webLocale()==Locale::GermanGermany;
    String c="<section class='card' id='remote-hardware-profiles' data-de='";
    c+=de?"1":"0";c+="' data-modules='";c+=options_.modules?"1":"0";c+="'><h2>";
    c+=de?"Hardware provisionieren":"Provision hardware";
    c+="</h2><style>.provisioning-details:not([hidden]){display:grid;grid-template-columns:minmax(0,1fr) minmax(240px,1fr);gap:1.2rem;align-items:start}.provisioning-preview img{display:block;margin:auto;object-fit:contain}#provision-current-fields{display:grid;grid-template-columns:minmax(100px,1fr) minmax(0,3fr);gap:4px 12px}#provision-current-fields dd{margin:0;overflow-wrap:anywhere}.provisioning-details fieldset{border:1px solid #8886;border-radius:8px;padding:1rem}@media(max-width:700px){.provisioning-details:not([hidden]){grid-template-columns:1fr}}</style><p>";
    c+=options_.externalTarget?(de?"Ziel: extern angeschlossener Prüfling.":"Target: externally connected device under test."):(de?"Ziel: dieser EnvNode.":"Target: this EnvNode.");
    c+="</p>";
    c+="<label>";c+=de?"Hardwaretyp":"Hardware type";
    c+="<select id='provision-hardware-kind'><option value='0'>Board</option>";
    if(options_.modules)c+="<option value='1'>"+String(de?"Modul":"Module")+"</option>";
    c+="</select></label><details><summary>";c+=de?"Profilkatalog":"Profile catalog";c+="</summary><p id='remote-profile-status'>";
    c+=de?"Eingebaute Profile verfügbar.":"Built-in profiles available.";
    c+="</p>";
    if(remoteProfilesEnabled_){c+="<button type='button' id='remote-profile-refresh'>";c+=de?"Online-Profile aktualisieren":"Refresh online profiles";c+="</button>";}
    c+="</details><form method='post' action='/hardware/board-identity/write' id='remote-profile-form'><div id='remote-module-fields' hidden><label>Slot<select name='remoteSlot' id='remote-profile-slot'><option>A</option><option>B</option></select></label></div><label>";
    c+=de?"Profil":"Profile";
    c+="<select name='remoteProfileKey' id='remote-profile-key' required><option value=''>";
    c+=de?"Profil auswählen":"Select profile";c+="</option>";
    for(size_t index=0;index<administration_.provisioningProfileCount();++index){
        ProvisioningBoardProfile p;if(!administration_.provisioningProfileAt(index,p))continue;
        const String revision=String(p.revision.major)+"."+String(p.revision.minor);
        c+="<option value='board:"+String(encodeBoardProfileId(p.id))+":"+revision+"' data-kind='0' data-name='"+escapeHtml(p.displayName)+"' data-revision='"+revision+"' data-minimum='";
        c+=String(p.minimumRuntimeFirmwareVersion.major)+"."+String(p.minimumRuntimeFirmwareVersion.minor)+"."+String(p.minimumRuntimeFirmwareVersion.patch);
        c+="' data-image='"+escapeHtml(p.imageUrl?p.imageUrl:"")+"'>"+escapeHtml(p.displayName)+" "+revision+"</option>";
    }
    if(options_.modules)c+=ProvisioningBuiltinModule;
    c+="</select></label><input type='hidden' name='boardProfileKey' id='provision-board-key'><input type='hidden' name='descriptorTemplate' value='duo-relay-0.3'>";
    c+="<input type='hidden' name='remoteProfileEntry' id='remote-profile-entry'><input type='hidden' name='remoteProfileData' id='remote-profile-data'>";
    c+="<input type='hidden' name='snapshotToken' id='provision-snapshot-token'><section class='notice'><h3>";
    c+=de?"Aktuell im EEPROM":"Currently in EEPROM";
    c+="</h3><p id='provision-current-status'></p><p id='provision-current-target'></p><dl id='provision-current-fields'></dl><div class='actions'><button type='button' id='provision-read'>";
    c+=de?"EEPROM erneut lesen":"Read EEPROM again";
    c+="</button><button type='button' id='provision-adopt' disabled>";
    c+=de?"Vorhandene Fertigungsdaten übernehmen":"Use existing manufacturing data";
    c+="</button></div></section><h3>";c+=de?"Neu zu schreiben":"To be written";c+="</h3>";
    c+="<div id='provision-details' class='provisioning-details' hidden><div class='provisioning-preview'><p id='remote-profile-minimum'></p><img id='remote-profile-image' hidden style='max-width:100%;max-height:360px' alt=''></div><div>";
    c+="<fieldset><legend>";c+=de?"Fertigungsdaten (optional)":"Manufacturing data (optional)";
    c+="</legend><label>";c+=de?"Seriennummer":"Serial number";
    c+="<input name='remoteSerial' id='remote-profile-serial' maxlength='17'></label><label>";
    c+=de?"Produktionscharge":"Production batch";
    c+="<input name='remoteBatch' id='remote-profile-batch' maxlength='64'></label><label>";
    c+=de?"Fertigungsdatum":"Production date";
    c+="<input name='remoteDate' id='remote-profile-date' type='date'></label></fieldset></div></div>";
    c+="<p class='notice' id='provision-comparison'></p><p id='provision-result' role='alert' aria-live='polite'></p>";
    c+="<label class='choice'><input type='checkbox' name='confirmRemoteProvisioning' id='remote-profile-confirm' value='1' required>";
    c+=de?"Ich bestätige das Schreiben des ausgewählten Profils ins EEPROM.":"I confirm writing the selected profile to EEPROM.";
    c+="</label><button type='submit' id='remote-profile-submit'>";c+=de?"Provisionieren":"Provision";
    c+="</button></form></section><script>";c+=ProvisioningDialogScript;c+="</script>";
    if(!remoteProfilesEnabled_)return c;
    c+="<script async crossorigin='anonymous' src='";
    c+=BrowserHardwareProfilesUrl;c+="' integrity='";c+=BrowserHardwareProfilesIntegrity;
    c+="' onerror=\"document.getElementById('remote-profile-status').textContent='";
    c+=de?"Browsercode nicht geladen. Eingebaute Profile bleiben verfügbar.":"Browser code unavailable. Built-in profiles remain available.";
    return c+"'\"></script>";
}
void HardwareProvisioningWebRoutes::handleRemoteProfileWrite(){
    if(!operationAllowed()||!remoteProfilesEnabled_)return;
    const bool de=pages_.webLocale()==Locale::GermanGermany;
    auto fail=[this,de](const char* detail){
        String message=de?"Profil konnte nicht provisioniert werden: ":"Profile could not be provisioned: ";message+=detail;
        sendResult("Provisioning","/hardware",message.c_str(),false);
    };
    if(server_.arg("confirmRemoteProvisioning")!="1"){fail("ConfirmationRequired");return;}
    const String key=server_.arg("remoteProfileKey");const char* separator=strchr(key.c_str(),'|');
    char id[96]={};
    if(!separator||separator-key.c_str()<=0||size_t(separator-key.c_str())>=sizeof(id)||strlen(separator+1)!=64){fail("InvalidSelection");return;}
    memcpy(id,key.c_str(),separator-key.c_str());
    std::unique_ptr<uint8_t[]> entryBytes,data;size_t entrySize=0,size=0;HardwareProfileEntry entry;
    if(!uploadedHex(server_.arg("remoteProfileEntry"),1024,entryBytes,entrySize)
        ||!decodeHardwareProfileEntry(entryBytes.get(),entrySize,entry)
        ||strcmp(id,entry.id)||strcmp(separator+1,entry.sha256)) {fail("InvalidProfileEntry");return;}
    entryBytes.reset();
    if(!uploadedHex(server_.arg("remoteProfileData"),MaximumHardwareProfileBytes,data,size)
        ||!validateHardwareProfilePayload(entry,data.get(),size)){fail("InvalidProfileData");return;}
    // HTTPS and SHA-256 are checked by the browser. This endpoint enforces the
    // payload's format/metadata, individual identity and verified EEPROM write.
    const String serial=server_.arg("remoteSerial");
    if(entry.kind==0){
        const String batch=server_.arg("remoteBatch"),date=server_.arg("remoteDate");
        if(batch.length()>64||date.length()>10){fail("InvalidManufacturingData");return;}
        BoardPortMap map;
        if(!BoardPortMapCodec::decode(data.get(),size,map)||!setBoardSerialNumber(map.serial,serial.c_str())){fail("InvalidBoardData");return;}
        memcpy(map.productionBatch,batch.c_str(),batch.length()+1);memcpy(map.productionDate,date.c_str(),date.length()+1);
        if(!verifySnapshot(false,ModuleSlot::A))return;
        const auto result=administration_.provisionBoardPortMap(map,true);
        logger_.infof("Browser board provisioning profile=%s result=%s",id,boardProvisioningStatusName(result.status));
        if(result.status!=BoardProvisioningStatus::Success){fail(boardProvisioningStatusName(result.status));return;}
    }else{
        const String slot=server_.arg("remoteSlot"),batch=server_.arg("remoteBatch"),date=server_.arg("remoteDate");
        if(!options_.modules||(slot!="A"&&slot!="B")||serial.length()>64||batch.length()>64||date.length()>10){fail("InvalidModuleData");return;}
        ModuleManufacturingData m;random_(m.instanceId,sizeof(m.instanceId));InstanceUuid::makeVersion4(m.instanceId);
        m.serialNumber=serial.c_str();m.productionBatch=batch.c_str();m.productionDate=date.c_str();
        std::unique_ptr<uint8_t[]> payload(new(std::nothrow) uint8_t[HardwareDescriptorStore::MaximumPayloadSize]);size_t payloadSize=0;
        if(!payload||!instantiateModuleProfile(data.get(),size,m,payload.get(),HardwareDescriptorStore::MaximumPayloadSize,payloadSize)){fail("InvalidModuleTemplate");return;}
        if(!verifySnapshot(true,slot=="A"?ModuleSlot::A:ModuleSlot::B))return;
        const auto result=administration_.provisionModuleDescriptor(slot=="A"?ModuleSlot::A:ModuleSlot::B,payload.get(),payloadSize,true);
        logger_.infof("Browser module provisioning profile=%s result=%s compatibility=%s",id,moduleDescriptorProvisioningStatusName(result.status),hardwareDescriptorCompatibilityStatusName(result.compatibilityStatus));
        if(result.status!=ModuleDescriptorProvisioningStatus::Success){fail(moduleDescriptorProvisioningStatusName(result.status));return;}
    }
    String message=de?"EEPROM geschrieben und verifiziert. ":"EEPROM written and verified. ";
    if(!options_.externalTarget)message+=de?"Neustarten, um die neue Hardwarekonfiguration zu übernehmen.":"Restart to apply the new hardware configuration.";
    sendResult("Provisioning","/hardware",message.c_str(),true);
}
}
