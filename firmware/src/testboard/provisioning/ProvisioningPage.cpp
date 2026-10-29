// EnvNode: see repository LICENSE.
#include "ProvisioningPage.h"
#include "HtmlEscaping.h"
#include "JsonWriter.h"
#include "BoardIdentityProfiles.h"
#include "testing/TestModel.h"
#include <cstring>
namespace EnvNode {
String buildProvisioningOverview(TestBoardProvisioning& provisioning, Locale locale) {
    const bool german = locale == Locale::GermanGermany;
    const auto current = provisioning.refresh();
    String c = "<section class='card'><h2>Provisioning</h2><p>";
    c += german ? "Ziel: extern angeschlossener Prüfling. Hardware auswählen, Fertigungsdaten ergänzen und EEPROM schreiben." : "Target: external device under test. Select hardware, enter manufacturing data and write EEPROM.";
    c += "</p><details><summary>";
    c += german ? "Anschluss und Hinweise" : "Connection and notes";
    c += "</summary><p>";
    c += german
        ? "Reales EEPROM des Prüflings an I²C: SDA GPIO16, SCL GPIO5, Board-Adresse 0x50, Moduladressen 0x52 (A) und 0x53 (B). ESP32 des Prüflings entfernen, andere Versorgungen sowie Mess-Pigtails und Input-Adapter trennen. Erst danach den separaten Provisioning-Stecker anschließen."
        : "Real target EEPROM on I²C: SDA GPIO16, SCL GPIO5, board address 0x50, module addresses 0x52 (A) and 0x53 (B). Remove the target ESP32 and disconnect other supplies, measurement pigtails and Input Adapter before connecting the separate provisioning cable.";
    c += "</p><p>";
    c += german ? "Nach dem Provisioning das Kabel vor der Durchgangsprüfung wieder abziehen. Die Firmware schaltet die Versorgung nicht ab." : "Disconnect the cable again before continuity testing. Firmware does not switch off its supply.";
    c += "</p></details><p>";
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
    return c + "</p></section>";
}
}
