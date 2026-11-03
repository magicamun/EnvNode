// EnvNode: see repository LICENSE.
#include "HardwareProvisioningWebRoutes.h"
#include "WebResult.h"
#include "HtmlEscaping.h"
#include "InstanceUuidWebText.h"
#include "InstanceUuid.h"
#include "DuoRelayDescriptor.h"
#include "BoardIdentityProfiles.h"
#include <cstring>
#include <cstdlib>
#include <memory>
#include <new>
namespace EnvNode {
HardwareProvisioningWebRoutes::HardwareProvisioningWebRoutes(WebServer& server,
    IWebPagePresenter& pages, ILogger& logger, IProvisioningAdministration& administration,
    Overview overview, Random random, ProvisioningWebOptions options)
    : server_(server), pages_(pages), logger_(logger), administration_(administration),
      overview_(overview), random_(random), options_(options) {}
void HardwareProvisioningWebRoutes::registerRoutes(WebServer& server) {
    if(remoteProfilesEnabled_) {
        server.on("/hardware/profile/state", HTTP_GET, [this]() { handleProfileState(); });
        server.on("/hardware/profile/write", HTTP_POST, [this]() { handleRemoteProfileWrite(); });
    }
    server.on("/hardware/eeprom", HTTP_GET, [this]() { handleEepromSnapshot(); });
    server.on("/hardware", HTTP_GET, [this]() { handleHardware(); });
    server.on("/hardware/board-identity/write", HTTP_POST, [this]() { handleBoardProvisioning(); });
    server.on("/device/board-identity/write", HTTP_POST, [this]() { handleBoardProvisioning(); });
    if (options_.modules) {
        server.on("/hardware/module-descriptor/write", HTTP_POST, [this]() { handleModuleDescriptorProvisioning(); });
        server.on("/device/module-descriptor/write", HTTP_POST, [this]() { handleModuleDescriptorProvisioning(); });
    }
}
const char* HardwareProvisioningWebRoutes::uiText(TextId id) const { return webText(pages_.webLocale(), id); }
void HardwareProvisioningWebRoutes::sendPage(const char* title, const char* route,
    const String& content, int status) { pages_.sendPage(title, route, content, status); }
void HardwareProvisioningWebRoutes::sendResult(const char* title, const char* route,
    const char* message, bool success) { sendWebResult(pages_, title, route, message, success); }
void HardwareProvisioningWebRoutes::handleHardware() {
    if (!operationAllowed()) return;
    String c = overview_();
    c += remoteProfilesSection();
    sendPage(uiText(TextId::Hardware), "/hardware", c);
}

bool HardwareProvisioningWebRoutes::operationAllowed() {
    if (!options_.available || options_.available()) return true;
    sendPage(uiText(TextId::Hardware), "/hardware",
        String("<section class='card'>") + uiText(TextId::ProvisioningLockedDuringConnectorTest) + "</section>", 409);
    return false;
}
void HardwareProvisioningWebRoutes::handleBoardProvisioning() {
    if (!operationAllowed()) return;
    ProvisioningBoardProfile profile;
    bool profileValid=false;
    if(server_.hasArg("boardProfileKey")) {
        // Explicit revision selection; never silently substitute a newer revision.
        const String key=server_.arg("boardProfileKey");
        uint16_t values[3]={};size_t part=0,digits=0;
        bool valid=!key.isEmpty() && !server_.hasArg("boardProfileId");
        for(size_t i=0;i<key.length() && valid;++i) {
            const char c=key[i];
            if(c>='0' && c<='9') {
                const unsigned maximum=part?255:UINT16_MAX;
                const unsigned next=static_cast<unsigned>(values[part])*10+(c-'0');
                valid=++digits<=5 && next<=maximum;
                if(valid)values[part]=next;
            } else if(digits && ((part==0 && c==':') || (part==1 && c=='.'))) { ++part;digits=0; }
            else valid=false;
        }
        BoardProfileId id=BoardProfileId::EnvNodeMainboard;
        profileValid=valid && part==2 && digits && decodeBoardProfileId(values[0],id)
            && administration_.provisioningProfile(id,{static_cast<uint8_t>(values[1]),static_cast<uint8_t>(values[2])},profile);
    } else {
        // Preserve the old API's original revision for integrations/older forms.
        const String value=server_.arg("boardProfileId");char* end=nullptr;
        const unsigned long parsed=strtoul(value.c_str(),&end,10);
        BoardProfileId id=BoardProfileId::EnvNodeMainboard;
        profileValid=!value.isEmpty() && end!=value.c_str() && !*end && parsed<=UINT16_MAX
            && decodeBoardProfileId(static_cast<uint16_t>(parsed),id) && administration_.provisioningProfile(id,profile);
    }
    const ProvisioningBoardProfile* requestedProfile=profileValid?&profile:nullptr;
    const String serialText = server_.hasArg("remoteSerial")?server_.arg("remoteSerial"):server_.arg("serialNumber");
    const bool serialValid = isValidBoardSerialNumber(serialText.c_str());
    if (requestedProfile == nullptr || !serialValid) {
        sendResult(uiText(TextId::BoardProvisioningFailed), "/hardware",
            uiText(TextId::TheSelectedBoardProfileOrBoardSerialNumber), false);
        return;
    }

    BoardIdentity requested = {
        requestedProfile->id,
        requestedProfile->revision,
        {},
    };
    setBoardSerialNumber(requested.serialNumber, serialText.c_str());
    const bool confirmed = server_.arg("confirmProvisioning")=="1"||server_.arg("confirmRemoteProvisioning")=="1";
    BoardProvisioningResult result;
    const String batch=server_.arg("remoteBatch"),date=server_.arg("remoteDate");
    if(batch.length()>64||date.length()>10){sendResult("Provisioning","/hardware","InvalidManufacturingData",false);return;}
    if(server_.hasArg("remoteBatch")||server_.hasArg("remoteDate")) {
        const auto* source=boardIdentityProfile(requested.profileId,requested.revision);BoardPortMap map;
        if(!source||!BoardPortMapCodec::decode(source->portMap,source->portMapSize,map)){sendResult("Provisioning","/hardware","InvalidBoardProfile",false);return;}
        map.serial=requested.serialNumber;memcpy(map.productionBatch,batch.c_str(),batch.length()+1);memcpy(map.productionDate,date.c_str(),date.length()+1);
        if(confirmed&&!verifySnapshot(false,ModuleSlot::A))return;
        result=administration_.provisionBoardPortMap(map,confirmed);
    } else {if(confirmed&&!verifySnapshot(false,ModuleSlot::A))return;result=administration_.provisionBoardIdentity(requested,confirmed);}
    logger_.infof("Board provisioning result=%s profile=%u revision=%u.%u serial=%s",
        boardProvisioningStatusName(result.status),
        static_cast<unsigned int>(requested.profileId),
        requested.revision.major,
        requested.revision.minor,
        requested.serialNumber.value);

    if (result.status == BoardProvisioningStatus::Success) {
        if (options_.externalTarget) {
            sendResult(uiText(TextId::BoardIdentityProvisioned), "/hardware",
                uiText(TextId::ExternalBoardIdentityWritten), true);
            return;
        }
        String content;
        content.reserve(600);
        content = (String("<div class='notice success'><strong>") + uiText(TextId::BoardIdentityWrittenAndVerified) + "</strong><p>" + uiText(TextId::TheActiveBoardProfileIsUnchangedForThisBoot) + "</p></div><form method='post' action='/restart' onsubmit='return confirm(globalThis.EnvNodeUi.RestartTheDeviceNow)'><button>" + uiText(TextId::RestartNow) + "</button></form><a class='button' href='/hardware'>" + uiText(TextId::RestartLater) + "</a>");
        sendPage(uiText(TextId::BoardIdentityProvisioned), "/hardware", content);
        return;
    }

    const char* message = uiText(TextId::BoardIdentityCouldNotBeWrittenAndVerified);
    switch (result.status) {
        case BoardProvisioningStatus::ConfirmationRequired:
            message = uiText(TextId::ExplicitConfirmationIsRequiredNoEEPROMDataWas);
            break;
        case BoardProvisioningStatus::InvalidIdentity:
            message = uiText(TextId::TheRequestedBoardIdentityIsNotSupportedNo);
            break;
        case BoardProvisioningStatus::WriteFailed:
            message = uiText(TextId::TheEEPROMDidNotAcceptTheWriteIt);
            break;
        case BoardProvisioningStatus::ReadbackFailed:
            message = uiText(TextId::TheEEPROMWriteCouldNotBeReadBack);
            break;
        case BoardProvisioningStatus::ReadbackInvalid:
            message = uiText(TextId::TheEEPROMReadbackFailedRecordValidationProvisioningWas);
            break;
        case BoardProvisioningStatus::ReadbackMismatch:
            message = uiText(TextId::TheEEPROMReadbackDidNotExactlyMatchThe);
            break;
        default:
            break;
    }
    sendResult(uiText(TextId::BoardProvisioningFailed), "/hardware", message, false);
}
void HardwareProvisioningWebRoutes::handleModuleDescriptorProvisioning() {
    if (!operationAllowed()) return;
    const String slotText = (server_.hasArg("remoteSlot")?server_.arg("remoteSlot"):server_.arg("descriptorModuleSlot"));
    ModuleSlot slot = ModuleSlot::A;
    const bool slotValid = slotText == "A" || slotText == "B";
    if (slotText == "B") slot = ModuleSlot::B;
    const bool templateValid =
        server_.arg("descriptorTemplate") == "duo-relay-0.3";

    DuoRelayDescriptorManufacturingData manufacturing;
    const String serialNumber = (server_.hasArg("remoteSerial")?server_.arg("remoteSerial"):server_.arg("descriptorSerialNumber"));
    const String productionBatch = (server_.hasArg("remoteBatch")?server_.arg("remoteBatch"):server_.arg("descriptorProductionBatch"));
    const String productionDate = (server_.hasArg("remoteDate")?server_.arg("remoteDate"):server_.arg("descriptorProductionDate"));
    const bool manufacturingValid = serialNumber.length() <= 64
        && productionBatch.length() <= 64
        && productionDate.length() <= 10;
    if (!slotValid || !templateValid || !manufacturingValid) {
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::TheSlotDescriptorTemplateOrManufacturingDataIs),
            false);
        return;
    }
    random_(manufacturing.instanceId, sizeof(manufacturing.instanceId));
    InstanceUuid::makeVersion4(manufacturing.instanceId);
    manufacturing.serialNumber = serialNumber.c_str();
    manufacturing.productionBatch = productionBatch.c_str();
    manufacturing.productionDate = productionDate.c_str();
    std::unique_ptr<uint8_t[]> payload(new (std::nothrow) uint8_t[DuoRelayDescriptor::MaximumEncodedSize]);
    if (!payload) {
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::PageRenderingFailedPleaseRetry), false);
        return;
    }
    size_t payloadSize = 0;
    const CompactCborStatus encodeStatus = DuoRelayDescriptor::encode(
        manufacturing, payload.get(), DuoRelayDescriptor::MaximumEncodedSize, payloadSize);
    if (encodeStatus != CompactCborStatus::Success) {
        logger_.errorf("DuoRelay descriptor encoding failed status=%s",
            compactCborStatusName(encodeStatus));
        sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware",
            uiText(TextId::TheDuoRelayDescriptorCouldNotBeEncodedNo),
            false);
        return;
    }

    const bool confirmed = (server_.hasArg("confirmModuleDescriptorProvisioning")||server_.arg("confirmRemoteProvisioning")=="1")
        && (server_.arg("confirmModuleDescriptorProvisioning")=="1"||server_.arg("confirmRemoteProvisioning")=="1");
    if(confirmed&&!verifySnapshot(true,slot))return;
    const ModuleDescriptorProvisioningResult result =
        administration_.provisionModuleDescriptor(
            slot, payload.get(), payloadSize, confirmed);
    logger_.infof(
        "Module descriptor provisioning result=%s slot=%s uuid=%s bytes=%u bank=%u generation=%lu decode=%s compatibility=%s",
        moduleDescriptorProvisioningStatusName(result.status), moduleSlotName(slot),
        descriptorUuid(manufacturing.instanceId).c_str(),
        static_cast<unsigned int>(payloadSize),
        static_cast<unsigned int>(result.bank),
        static_cast<unsigned long>(result.generation),
        hardwareDescriptorDecodeStatusName(result.decodeStatus),
        hardwareDescriptorCompatibilityStatusName(result.compatibilityStatus));

    if (result.status == ModuleDescriptorProvisioningStatus::Success) {
        const char* bank = result.bank == HardwareDescriptorBank::B ? "B" : "A";
        String message = (String(uiText(TextId::TheDuoRelayDescriptorWasEncodedValidatedWrittenTo)) + " ");
        message += bank;
        message += (String(uiText(TextId::VerifiedAndRediscoveredWithInstanceUuid)) + " ");
        message += descriptorUuid(manufacturing.instanceId);
        message += uiText(TextId::NoRestartRequired);
        sendResult(uiText(TextId::ModuleDescriptorProvisioned), "/hardware", message.c_str(), true);
        return;
    }

    const char* message = uiText(TextId::TheModuleDescriptorCouldNotBeWrittenAnd);
    switch (result.status) {
        case ModuleDescriptorProvisioningStatus::ConfirmationRequired:
            message = uiText(TextId::ExplicitConfirmationIsRequiredNoEEPROMDataWas);
            break;
        case ModuleDescriptorProvisioningStatus::InvalidSlot:
            message = uiText(TextId::TheSelectedModuleSlotIsInvalidNoEEPROM);
            break;
        case ModuleDescriptorProvisioningStatus::InvalidDescriptor:
            message = uiText(TextId::TheGeneratedDescriptorFailedSemanticValidationNoEEPROM);
            break;
        case ModuleDescriptorProvisioningStatus::IncompatibleDescriptor:
            message = uiText(TextId::TheDescriptorIsNotCompatibleWithTheActive);
            break;
        case ModuleDescriptorProvisioningStatus::StorageUnavailable:
            message = uiText(TextId::TheSelectedModuleEEPROMIsUnavailableCheckThat);
            break;
        case ModuleDescriptorProvisioningStatus::WriteFailed:
            message = uiText(TextId::TheSelectedModuleEEPROMDidNotAcceptThe);
            break;
        case ModuleDescriptorProvisioningStatus::VerificationFailed:
            message = uiText(TextId::TheDescriptorWriteFailedAtomicBankReadbackVerification);
            break;
        case ModuleDescriptorProvisioningStatus::RediscoveryFailed:
            message = uiText(TextId::TheDescriptorWasWrittenAndVerifiedButSemantic);
            break;
        default:
            break;
    }
    sendResult(uiText(TextId::ModuleDescriptorProvisioningFailed), "/hardware", message, false);
}
}
