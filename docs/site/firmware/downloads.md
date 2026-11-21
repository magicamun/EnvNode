# Firmware herunterladen

Hier findest du die veröffentlichten Versionen für EnvNode und den separaten Tester.
Pro Build-Ziel wird jede neue Firmware-Version einmal veröffentlicht, einschließlich
Patch-Versionen wie `0.8.6`. Weitere Commits ohne Versionsänderung ersetzen weder
das veröffentlichte Image noch seine Änderungsbeschreibung. Commit und SHA-256
identifizieren den veröffentlichten Stand. Neuere Entwicklungsbuilds derselben
Version stehen lokal unter `firmware/bin/` zur Verfügung.

Die Dateien sind **Anwendungsimages für Web-OTA** auf dem klassischen ESP32
(`esp32dev`). Verwende das Image des passenden Build-Ziels. Für die erste Installation
über USB werden zusätzliche Flashbestandteile benötigt; siehe
[Bauen und Aufspielen](installation.md) und [Wartung und OTA](maintenance.md).
## Boardrevision und Firmware wählen

Wähle beim Provisioning das Profil für die tatsächliche PCB-Revision.
Die folgenden Versionen markieren die dokumentierte Profil- oder
Funktionsunterstützung. Der Prüfstand steht in der letzten Spalte.

| Hardware | EnvNode | Tester | Erprobung |
| --- | --- | --- | --- |
| EnvNode Mini 0.10.0 und WeatherStation 0.4 | ab 0.8.11 | Prüfprofile ab 0.15.0 | Physische Abnahme der neuen Mainboardrevisionen offen |
| Test-Controller 0.2 und Input-Adapter 0.3 | – | Gemeinsamer Test- und Provisioningstand ab 0.13.0. Reset-Selbsttest ab 0.14.0 | Abnahme der Kombination und Schreiben externer Modul-EEPROMs offen |
| RainDetectorController 0.4 und Head 0.2 | integrierter Betrieb ab 0.8.12 | – | Benetzung, Regenende und Trocknung am Versuchsaufbau geprüft. Feldbetrieb und Taupunktheizung am Zielaufbau offen |
| SCT013Head 0.2 | SCT013-DMA-Erfassung ab 0.8.14 | – | DMA-Betrieb am Mini erprobt. Dauertest und Wiederanlauf weiter beobachten |

Weitere Revisionen und die Mindeststände der einzelnen Funktionen stehen in der
[Kompatibilitätsübersicht](../compatibility.md). Halte bei der Hardwareabnahme
die PCB-Revision, das aktive Profil und den getesteten Firmwarestand mit Commit
fest. Bei einem veröffentlichten Build findest du Commit und SHA-256 im
Downloadkatalog.

Die Downloads unterliegen der [EnvNode-Lizenz](../license.md).
Die Remote-Node erhält später ein eigenes Build-Ziel mit eigener Versionsreihe.

## EnvNode

Noch kein Build veröffentlicht.

## Tester

Noch kein Build veröffentlicht.
