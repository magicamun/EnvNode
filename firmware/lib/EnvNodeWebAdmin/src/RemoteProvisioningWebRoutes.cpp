// EnvNode: see repository LICENSE.
#include "HardwareProvisioningWebRoutes.h"
#include "HardwareProfileHttpsTransport.h"
#include "ModuleProfileTemplate.h"
#include "HtmlEscaping.h"
#include "InstanceUuid.h"
#include <cstring>
#include <new>
namespace EnvNode {
bool HardwareProvisioningWebRoutes::enableRemoteProfiles() {
    if(profiles_)return true;
#ifdef ARDUINO
    profileTransport_.reset(new(std::nothrow) HardwareProfileHttpsTransport(logger_));
    if(profileTransport_)ownedProfiles_.reset(new(std::nothrow) HardwareProfileRepository(*profileTransport_));
    profiles_=ownedProfiles_.get();
#endif
    return profiles_!=nullptr;
}
String HardwareProvisioningWebRoutes::remoteProfilesSection(){
    const bool de=pages_.webLocale()==Locale::GermanGermany;
    String c="<section class='card'><h2>";
    c+=de?"Nachladbare Hardwareprofile":"Downloadable hardware profiles";
    c+="</h2><p>";
    c+=options_.externalTarget
        ? (de?"Ziel: extern angeschlossener Prüfling.":"Target: externally connected device under test.")
        : (de?"Ziel: dieser EnvNode.":"Target: this EnvNode.");
    c+="</p><p>Status: ";c+=profiles_->status();c+="</p>";
    c+="<form method='post' action='/hardware/profiles/refresh'><button>";
    c+=de?"Profile aktualisieren":"Refresh profiles";c+="</button></form>";
    if(!profiles_->count()) {
        c+="<p>";c+=de?"Kein Remote-Katalog geladen. Die eingebauten Profile unten bleiben verfügbar. WLAN und Uhrzeit müssen bereit sein.":"No remote catalog loaded. Built-in profiles below remain available. Wi-Fi and synchronized time are required.";
        return c+"</p></section>";
    }
    c+="<form method='post' action='/hardware/profile/write'><label>";
    c+=de?"Profil":"Profile";
    c+="<select name='remoteProfileKey' id='remote-profile-key' required>";
    for(size_t i=0;i<profiles_->count();++i){const auto& e=*profiles_->at(i);
        if(e.kind&&!options_.modules)continue;
        c+="<option value='"+escapeHtml(e.id)+"|"+e.sha256+"' data-kind='"+String(e.kind)+"' data-minimum='"
            +String(e.minimumFirmware.major)+"."+String(e.minimumFirmware.minor)+"."+String(e.minimumFirmware.patch)+"' data-image='";
        if(e.image[0])c+=escapeHtml((String("https://docs.envnode.de/")+e.image).c_str());
        c+="'>"+escapeHtml(e.name)+" "+String(e.revision.major)+"."+String(e.revision.minor)+"</option>";
    }
    c+="</select></label><p id='remote-profile-minimum'></p><img id='remote-profile-image' hidden style='max-width:100%;max-height:520px' alt=''>";
    c+="<div id='remote-module-fields'><label>Slot<select name='remoteSlot'><option>A</option><option>B</option></select></label>";
    c+="<label>";c+=de?"Produktionscharge":"Production batch";
    c+="<input name='remoteBatch' maxlength='64'></label><label>";c+=de?"Produktionsdatum":"Production date";
    c+="<input name='remoteDate' type='date'></label></div><label>";c+=de?"Seriennummer (optional)":"Serial number (optional)";
    c+="<input name='remoteSerial' id='remote-profile-serial' maxlength='17'></label>";
    c+="<label class='choice'><input type='checkbox' name='confirmRemoteProvisioning' value='1' required>";
    c+=de?"Ich bestätige das Schreiben des ausgewählten Profils ins EEPROM.":"I confirm writing the selected profile to EEPROM.";
    c+="</label><button type='submit'>";c+=de?"Provisionieren":"Provision";
    c+="</button></form></section><script>";
    c+=de?"const hardwareMinimumLabel='Betriebs-Firmware ab ';":"const hardwareMinimumLabel='Runtime firmware ≥ ';";
    c+=R"JS(document.addEventListener('DOMContentLoaded',()=>{const s=document.getElementById('remote-profile-key'),i=document.getElementById('remote-profile-image');if(!s)return;function update(){const o=s.selectedOptions[0];if(!o)return;const module=o.dataset.kind==='1';document.getElementById('remote-module-fields').hidden=!module;document.getElementById('remote-profile-serial').maxLength=module?64:17;document.getElementById('remote-profile-minimum').textContent=hardwareMinimumLabel+o.dataset.minimum;i.hidden=true;i.onload=()=>{i.hidden=false};i.onerror=()=>{i.hidden=true};if(o.dataset.image){i.alt=o.textContent;i.src=o.dataset.image;}else i.removeAttribute('src');}s.addEventListener('change',update);update();});</script>)JS";
    return c;
}
void HardwareProvisioningWebRoutes::handleProfileRefresh(){
    if(!operationAllowed()||!profiles_)return;
    profiles_->refresh();handleHardware();
}
void HardwareProvisioningWebRoutes::handleRemoteProfileWrite(){
    if(!operationAllowed()||!profiles_)return;
    const bool de=pages_.webLocale()==Locale::GermanGermany;
    const char* title=de?"Provisioning":"Provisioning";
    auto fail=[this,title,de](const char* detail){
        String message=de?"Profil konnte nicht provisioniert werden: ":"Profile could not be provisioned: ";message+=detail;
        sendResult(title,"/hardware",message.c_str(),false);
    };
    if(server_.arg("confirmRemoteProvisioning")!="1"){fail("ConfirmationRequired");return;}
    const String key=server_.arg("remoteProfileKey");const char* separator=strchr(key.c_str(),'|');
    char id[96]={};
    if(!separator||separator-key.c_str()<=0||size_t(separator-key.c_str())>=sizeof(id)||strlen(separator+1)!=64){fail("InvalidSelection");return;}
    memcpy(id,key.c_str(),separator-key.c_str());
    const auto* selected=profiles_->load(id,separator+1);
    if(!selected){fail(profiles_->status());return;}
    const String serial=server_.arg("remoteSerial");
    if(selected->entry.kind==0){
        BoardPortMap map;
        if(!BoardPortMapCodec::decode(selected->bytes.get(),selected->size,map)||!setBoardSerialNumber(map.serial,serial.c_str())){fail("InvalidBoardData");return;}
        const auto result=administration_.provisionBoardPortMap(map,true);
        logger_.infof("Remote board provisioning profile=%s result=%s",id,boardProvisioningStatusName(result.status));
        if(result.status!=BoardProvisioningStatus::Success){fail(boardProvisioningStatusName(result.status));return;}
    }else{
        const String slot=server_.arg("remoteSlot"),batch=server_.arg("remoteBatch"),date=server_.arg("remoteDate");
        if(!options_.modules||(slot!="A"&&slot!="B")||serial.length()>64||batch.length()>64||date.length()>10){fail("InvalidModuleData");return;}
        ModuleManufacturingData m;random_(m.instanceId,sizeof(m.instanceId));InstanceUuid::makeVersion4(m.instanceId);
        m.serialNumber=serial.c_str();m.productionBatch=batch.c_str();m.productionDate=date.c_str();
        std::unique_ptr<uint8_t[]> payload(new(std::nothrow) uint8_t[HardwareDescriptorStore::MaximumPayloadSize]);size_t size=0;
        if(!payload||!instantiateModuleProfile(selected->bytes.get(),selected->size,m,payload.get(),HardwareDescriptorStore::MaximumPayloadSize,size)){fail("InvalidModuleTemplate");return;}
        const auto result=administration_.provisionModuleDescriptor(slot=="A"?ModuleSlot::A:ModuleSlot::B,payload.get(),size,true);
        logger_.infof("Remote module provisioning profile=%s result=%s compatibility=%s",id,moduleDescriptorProvisioningStatusName(result.status),hardwareDescriptorCompatibilityStatusName(result.compatibilityStatus));
        if(result.status!=ModuleDescriptorProvisioningStatus::Success){fail(moduleDescriptorProvisioningStatusName(result.status));return;}
    }
    String message=de?"EEPROM geschrieben und verifiziert. ":"EEPROM written and verified. ";
    if(!options_.externalTarget)message+=de?"Neustarten, um die neue Hardwarekonfiguration zu übernehmen.":"Restart to apply the new hardware configuration.";
    sendResult(title,"/hardware",message.c_str(),true);
}
}
