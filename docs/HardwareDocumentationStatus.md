# Hardware-Dokumentationsstand

Stand: **28. Oktober 2026**, nach redaktioneller Freigabe durch den Projektinhaber.
„Abgeschlossen“ bezeichnet die Dokumentation. Fertigung, Inbetriebnahme und
Hardwarequalifikation werden in jeder Anleitung getrennt geführt.

## Abgeschlossen und aktuell

| Board | Revision | Dokumentation |
| --- | --- | --- |
| EnvNode Mini | 0.9.0 | Web + PDF; TP1, steckbare Pull-ups, C2 und neue I²C-Referenzen |
| Weatherstation | 0.3.0 | Web + PDF; TP1, steckbare Pull-ups und C2 |
| ORing Power | 0.1.1 | Web + PDF, Basis 0.1 mit Ergänzung |
| DuoRelay | 0.3 | Web + PDF |
| AnalogHydroPressure | 0.6 | Web + PDF; steckbare Bus-/Adresswahl, neue L1, THT-Shunt |
| SD-Card | 0.1 | Web + PDF; Board technisch festgelegt, Fertigung/Aufbau folgen; Firmware-Mindestversion später ergänzen |
| RainDetectorController | 0.4 | Web + PDF, neue Kontur und Renderings |
| RainDetectorHead | 0.2 | Web + PDF, neue Kontur und Renderings |
| Test-Controller | 0.1 | Neue vollständige Web-/PDF-Aufbauanleitung |
| Input-Adapter | 0.1 | Neue vollständige Web-/PDF-Aufbauanleitung mit 40 Messkontakten |
| Modul-Adapter | 0.1 | Neue vollständige Web-/PDF-Aufbauanleitung |

Die vorher als „ausgearbeitet“ geführten sieben Anleitungen sind durch
Nutzerentscheidung redaktionell **abgeschlossen**. Abgelöste Stände bleiben
abgeschlossen und stehen unter „Alte Revisionen“: Mini 0.8.1, Weather 0.2.1,
AnalogHydroPressure 0.5, RainDetectorController 0.3 und RainDetectorHead 0.1.
Noch ältere vorhandene PDFs bleiben dort ebenfalls erreichbar.

## Teilumfang

| Board | Revision | Noch zu ergänzen |
| --- | --- | --- |
| SCT013Head | 0.1 | Sensorbetrieb und Buchsenmechanik; bislang Erstaufbau und Nullmessung |
| I²C Carrier HalfSize | 0.1 | Vollständiger Bring-up; bislang Montage und Jumper |

## Fehlende Anleitungen

| Board | Stand |
| --- | --- |
| SX1262-Remote | Eigenständige Anleitung fehlt; Revisionsbezug zuerst eindeutig festlegen |
| SolarRadiation | Eigenständige Anleitung fehlt |
| RemoteNode | Revisions-/Dokumentationsumfang und Anleitung fehlen |

Die monolithische Test-Provisioning-Platine ist durch Test-Controller,
Input-Adapter und Modul-Adapter abgelöst. Keine neue Anleitung dafür eingeplant.
Bibliothekskopien, Empty-Modulvorlagen und DesignBlocks sind keine zusätzlichen
fehlenden Boardanleitungen.

## Quellen, Ablage und Prüfung

Die neuen Anleitungen sind unter [Aufbauanleitungen](site/guides/index.md), die
historischen Stände unter [Alte Revisionen](site/guides/old-revisions.md) verlinkt.
Die Basisinventur bleibt als [historischer Audit](HardwareDocumentationAudit-2026-10-28.md)
erhalten; dieser Statusbericht ersetzt dessen damalige Einstufungen.

Mini und Weather verwenden den gespeicherten main-Arbeitsstand. AnalogHydroPressure,
SD-Card, RainDetector und die Testboards verwenden die im jeweiligen
Rendernachweis benannten Arbeitszweige und SHA-256-Quellstände. Es wurden keine
Hardwarezweige zusammengeführt und keine KiCad-Quelldateien für die
Dokumentation geändert.

Neu: 18 native KiCad-Renderings für neun Boards, mit Quell-/Bildprüfsummen.
Bei allen neun Boards stimmen die gemeinsamen belegten PCB-Pins mit den neu
exportierten Schaltplan-Netzlisten überein. Kein neuer vollständiger ERC-/DRC-
oder physischer Hardwaretest wird daraus abgeleitet.

Die vollständige Website einschließlich 17 aktueller/archivierter PDF-Ausgaben
wird mit `mkdocs build --strict` geprüft. Die neue PDF-Laufzeit liegt lokal isoliert
unter /private/tmp; der CI-Workflow stellt Pango und DejaVu weiterhin selbst bereit.
Details zur Erstellung und lokalen Vorschau: [Website bearbeiten](WEBSITE.md).

## Nächster separater Schritt

Firmware pro Versionscommit dokumentieren und passende Binärdateien veröffentlichen;
SD-Card anschließend die geprüfte Mindestversion zuordnen. Diese Arbeit ist
vorgemerkt und in dieser Hardware-Dokumentationsrunde nicht implementiert.

## Einheitliche Bildfolge in den Aufbauanleitungen

Alle 18 gepflegten aktuellen und archivierten Web-Anleitungen zeigen die
3D-Vorderseite vor der Teileliste. Zwölf tatsächlich bestückte Rückseiten erhalten
zusätzlich eine 3D-Rückansicht. Die Bestückungsübersichten enthalten insgesamt
30 senkrechte farbige KiCad-Renderings ohne Bauteilmodelle. Die Mainboard-Pläne
liegen als eigenständige PNGs für die spätere Testsoftware vor.
Die 17 PDF-Ausgaben verwenden dieselben Bilder.

Die Bildfolge ist dauerhaft in `HardwareDocumentationStyle.md` festgelegt;
Manifest und Generator stehen unter `docs/board-views.yml` und
`tools/docs_board_views.py`. Alle ursprünglichen KiCad-Quelldateien bleiben
unverändert.
