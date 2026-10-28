// EnvNode: see repository LICENSE.
#include "ProvisioningPage.h"
#include "HtmlEscaping.h"
namespace EnvNode {
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
    c += german ? "Unterstützt: Mini 0.8, Weather 0.2 und Mainboard 0.3. Mini 0.9 und Weather 0.3 folgen mit der Katalogerweiterung. Nicht als ältere Revision provisionieren." : "Supported: Mini 0.8, Weather 0.2 and Mainboard 0.3. Mini 0.9 and Weather 0.3 require a catalog update. Do not provision them as older revisions.";
    return c + "</p></section>";
}
}
