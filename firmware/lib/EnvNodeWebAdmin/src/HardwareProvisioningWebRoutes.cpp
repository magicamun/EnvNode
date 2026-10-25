// EnvNode: see repository LICENSE.
#include "HardwareProvisioningWebRoutes.h"
#include "WebResult.h"
#include "HtmlEscaping.h"
#include "InstanceUuidWebText.h"
#include "InstanceUuid.h"
#include "DuoRelayDescriptor.h"
#include <cstdlib>
#include <memory>
#include <new>
namespace EnvNode {
HardwareProvisioningWebRoutes::HardwareProvisioningWebRoutes(WebServer& server,
    IWebPagePresenter& pages, ILogger& logger, IProvisioningAdministration& administration,
    Overview overview, Random random)
    : server_(server), pages_(pages), logger_(logger), administration_(administration),
      overview_(overview), random_(random) {}
void HardwareProvisioningWebRoutes::registerRoutes(WebServer& server) {
    server.on("/hardware", HTTP_GET, [this]() { handleHardware(); });
    server.on("/hardware/board-identity/write", HTTP_POST, [this]() { handleBoardProvisioning(); });
    server.on("/device/board-identity/write", HTTP_POST, [this]() { handleBoardProvisioning(); });
    server.on("/hardware/module-descriptor/write", HTTP_POST, [this]() { handleModuleDescriptorProvisioning(); });
    server.on("/device/module-descriptor/write", HTTP_POST, [this]() { handleModuleDescriptorProvisioning(); });
}
const char* HardwareProvisioningWebRoutes::uiText(TextId id) const { return webText(pages_.webLocale(), id); }
void HardwareProvisioningWebRoutes::sendPage(const char* title, const char* route,
    const String& content, int status) { pages_.sendPage(title, route, content, status); }
void HardwareProvisioningWebRoutes::sendResult(const char* title, const char* route,
    const char* message, bool success) { sendWebResult(pages_, title, route, message, success); }
void HardwareProvisioningWebRoutes::handleHardware() {
    String c = overview_();
    const BoardIdentity identity = administration_.provisioningBoardIdentity();
    c += (String("<section class='card'><h2>") + uiText(TextId::ProvisionModuleDescriptor) + "</h2>");
    c += (String("<div class='notice'><strong>ENHD descriptor 0.1</strong><p>") + uiText(TextId::ThisCreatesTheDeterministicDuoRelay03CBOR) + "</p></div>");
    c += "<form method='post' action='/hardware/module-descriptor/write' onsubmit='return confirm(globalThis.EnvNodeUi.WriteAndVerifyTheDuorelayDescriptorInThe)'>";
    c += (String("<label>") + uiText(TextId::ModuleSlot) + "<select name='descriptorModuleSlot' required><option value='A'>A</option><option value='B'>B</option></select></label>");
    c += (String("<label>") + uiText(TextId::DescriptorTemplate) + "<select name='descriptorTemplate' required><option value='duo-relay-0.3'>EnvNode DuoRelay 0.3</option></select></label>");
    c += (String("<p class='help'>") + uiText(TextId::APersistentUUIDVersion4IsGeneratedAutomatically) + "</p>");
    c += (String("<label>") + uiText(TextId::SerialNumberOptional) + "<input name='descriptorSerialNumber' maxlength='64'></label>");
    c += (String("<label>") + uiText(TextId::ProductionBatchOptional) + "<input name='descriptorProductionBatch' maxlength='64'></label>");
    c += (String("<label>") + uiText(TextId::ProductionDateOptional) + "<input type='date' name='descriptorProductionDate'></label>");
    c += (String("<label class='choice'><input type='checkbox' name='confirmModuleDescriptorProvisioning' value='1' required>") + uiText(TextId::IUnderstandThatThisWritesASelfDescribing) + "</label>");
    c += (String("<div class='actions'><button class='danger' type='submit'>") + uiText(TextId::WriteDuoRelayDescriptor) + "</button></div></form></section>");
    c += (String("<section class='card'><h2>") + uiText(TextId::ProvisionBoardIdentity) + "</h2>");
    c += (String("<div class='notice'><strong>") + uiText(TextId::AdvancedOperation) + "</strong><p>" + uiText(TextId::ThisWritesPermanentPhysicalBoardIdentityDataThe) + "</p></div>");
    c += "<form method='post' action='/hardware/board-identity/write' onsubmit='return confirm(globalThis.EnvNodeUi.WriteAndVerifyThisBoardIdentityTheActive)'>";
    c += (String("<label>") + uiText(TextId::BoardProfile) + "<select name='boardProfileId' required>");
    for (size_t index = 0; index < administration_.provisioningProfileCount(); ++index) {
        ProvisioningBoardProfile profileData;
        if (!administration_.provisioningProfileAt(index, profileData)) continue;
        const ProvisioningBoardProfile* profile = &profileData;
        c += "<option value='" + String(encodeBoardProfileId(profile->id)) + "'";
        if (profile->id == identity.profileId) c += " selected";
        c += ">" + escapeHtml(profile->displayName) + " "
            + String(profile->revision.major) + "." + String(profile->revision.minor)
            + "</option>";
    }
    c += "</select></label>";
    c += (String("<p class='help'>") + uiText(TextId::TheSelectedBoardNameIsStoredAsIts) + "</p>");
    c += (String("<label>") + uiText(TextId::BoardSerialNumber) + "<input name='serialNumber' maxlength='") + String(MaxBoardSerialNumberLength) + "' pattern='[A-Za-z0-9._-]*' value='" + escapeHtml(identity.serialNumber.value) + "'></label>";
    c += (String("<p class='help'>") + uiText(TextId::SerialNumber0ExplicitlyMeansUnassigned) + "</p>");
    c += (String("<label class='choice'><input type='checkbox' name='confirmProvisioning' value='1' required>") + uiText(TextId::IUnderstandThatThisWritesManufacturingIdentityData) + "</label>");
    c += (String("<div class='actions'><button class='danger' type='submit'>") + uiText(TextId::WriteBoardIdentity) + "</button></div></form></section>");
    sendPage(uiText(TextId::Hardware), "/hardware", c);
}

void HardwareProvisioningWebRoutes::handleBoardProvisioning() {
    const String profileText = server_.arg("boardProfileId");
    char* profileEnd = nullptr;
    const unsigned long parsedProfile = strtoul(profileText.c_str(), &profileEnd, 10);
    BoardProfileId profileId = BoardProfileId::EnvNodeMainboard;
    const bool profileValid = !profileText.isEmpty()
        && profileEnd != profileText.c_str()
        && *profileEnd == '\0'
        && parsedProfile <= UINT16_MAX
        && decodeBoardProfileId(static_cast<uint16_t>(parsedProfile), profileId);
    ProvisioningBoardProfile profile;
    const ProvisioningBoardProfile* requestedProfile = profileValid
        && administration_.provisioningProfile(profileId, profile) ? &profile : nullptr;
    const String serialText = server_.arg("serialNumber");
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
    const bool confirmed = server_.hasArg("confirmProvisioning")
        && server_.arg("confirmProvisioning") == "1";
    const BoardProvisioningResult result =
        administration_.provisionBoardIdentity(requested, confirmed);
    logger_.infof("Board provisioning result=%s profile=%u revision=%u.%u serial=%s",
        boardProvisioningStatusName(result.status),
        static_cast<unsigned int>(requested.profileId),
        requested.revision.major,
        requested.revision.minor,
        requested.serialNumber.value);

    if (result.status == BoardProvisioningStatus::Success) {
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
    const String slotText = server_.arg("descriptorModuleSlot");
    ModuleSlot slot = ModuleSlot::A;
    const bool slotValid = slotText == "A" || slotText == "B";
    if (slotText == "B") slot = ModuleSlot::B;
    const bool templateValid =
        server_.arg("descriptorTemplate") == "duo-relay-0.3";

    DuoRelayDescriptorManufacturingData manufacturing;
    const String serialNumber = server_.arg("descriptorSerialNumber");
    const String productionBatch = server_.arg("descriptorProductionBatch");
    const String productionDate = server_.arg("descriptorProductionDate");
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

    const bool confirmed = server_.hasArg("confirmModuleDescriptorProvisioning")
        && server_.arg("confirmModuleDescriptorProvisioning") == "1";
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
