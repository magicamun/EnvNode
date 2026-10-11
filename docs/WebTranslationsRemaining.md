<!-- SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0 -->
<!-- See the repository root LICENSE. -->
# Oberflächentexte: Übersetzung und Ausnahmen

Die Zuordnung ist in [WebLocalization.md](WebLocalization.md) festgelegt.
Alle erfassten Bedien-, Hilfe-, Bestätigungs- und Fehlertexte verwenden stabile
Text-IDs im zentralen `firmware/include/WebTextCatalog.inc`. Die gespeicherte
Locale wählt Deutsch oder den englischen Fallback. Browsertexte verwenden eine
kleine, vom Server gelieferte Auswahl desselben Katalogs.

Dieser Bericht prüft statische HTML-Texte, direkt verwendete Seiten-/Meldungstexte
und Texte des Display-Editors. Aufgeteilte Texte werden als Fragmente erfasst.
Indirekte Messgrößen-, Qualitäts-, Implementierungs- und Formatfehlertexte sind
in `WebPresentationText` typisiert angebunden. Die Klassifizierung der übrigen
technischen Daten steht unten; sie sind kein Übersetzungsrückstand.

Regenerieren: `python3 firmware/scripts/report_web_translations.py`.

## Noch zu übersetzen (0 Fundstellen)

Keine Fundstellen in den geprüften Webansichten.


## Katalogeintrag vorhanden, noch anzubinden (0 Fundstellen)

Keine Fundstellen in den geprüften Webansichten.


## Nicht zu übersetzen

| Kategorie | Stellen / Beispiele | Grund |
|---|---|---|
| HTTP- und Formularschnittstelle | WebService: `/time/save`, `server_.arg("locale")`, `Cache-Control`; ValueWebView: `name='code'` | Stabile Schnittstellen, keine Beschriftungen |
| DOM und Browserlogik | PageEditorScript: `page-formats`, `data-property-source`, `selectorChoice`, Formular- und JSON-Schlüssel | Identifikatoren für die Programmlogik |
| Eingaben und Nutzerdaten | `cfg.device.name`, `slot.name`, `d.name`, `option.label`, `contentPage.formats`, boolesche und Enum-Displaytexte | Vom Benutzer festgelegte Inhalte |
| Diagnoseprotokolle | `logger_.*`, LogWebView: `entry.message`, `entry.level`, `buildRecentLogJson()` | Rohdiagnose und Log-Level-Codes bleiben geräteweit konsistent |
| Technische Statuscodes | `boardIdentityStatusName`, `hardwareDescriptorDecodeStatusName`, `hardwareDescriptorStoreStatusName`, `hardwareDescriptorCompatibilityStatusName`, `moduleDeviceInventoryStatusName`, Herkunftscodes | Rohcodes wie `InvalidCRC` oder `ResourceUnavailable`, auch für Diagnose und Support |
| Hardware und Identitäten | Platinen-/Sensormodell, Treiber-ID, Modulslot A/B, UUID, EEPROM-Bank, `org.envnode.driver.*` | Identifikation statt sprachlicher Beschriftung |
| Werte und Einheiten | Messwerte, GPIO-/I2C-Adressen, `hPa`, `kPa`, `inHg`, `ms`, `SDA`, `SCL` | Physische Werte und standardisierte Symbole |
| Locale und Zeitzonen | `de-DE`, `en-GB`, `en-US`, `Deutsch (de-DE)`, `English (en-GB)`, `Europe/Berlin`, POSIX-Zeitzonenwerte | Stabile Locale-/Zeitzonenwerte und Sprachen in Eigenbezeichnung |
| Formatierung und Firmware | `%f`, `%u`, `%s`, `%%`, Firmware-/Build-Kennungen, `.pio/build/.../firmware.bin` | Technische Syntax und Artefaktkennungen |
| Beispiel-Nutzerdaten | Wasserquelle, Automatik, Zisterne, Hauswasser in WebServiceValues | Beispiel einer gespeicherten Wertdefinition, keine UI-Beschriftung |

### Direkt sichtbare technische Texte

Diese festen Texte bleiben bewusst unverändert. Sprachliche Beschriftungen
neben ihnen werden übersetzt.

- `firmware/src/PropertyPreviewWebView.cpp:71`: `Format`
- `firmware/src/PropertyPreviewWebView.cpp:209`: `Format`
- `firmware/src/WebService.cpp:705`: `EnvNode · v`
- `firmware/src/WebService.cpp:745`: `Build`
- `firmware/src/WebService.cpp:745`: `Flash`
- `firmware/src/WebService.cpp:747`: `SSID`
- `firmware/src/WebService.cpp:751`: `RSSI`
- `firmware/src/WebService.cpp:784`: `DHCP`
- `firmware/src/WebService.cpp:796`: `Home Assistant Discovery`
- `firmware/src/WebService.cpp:848`: `EEPROM`
- `firmware/src/WebService.cpp:960`: `ENHD descriptor 0.1`
- `firmware/src/WebService.cpp:963`: `EnvNode DuoRelay 0.3`
- `firmware/src/WebService.cpp:1669`: `GPIO`
- `firmware/src/WebService.cpp:1768`: `GPIO`
- `firmware/src/WebService.cpp:2028`: `MCU`
- `firmware/src/WebService.cpp:2028`: `WiFi`
- `firmware/src/WebService.cpp:2036`: `SDA GPIO`
