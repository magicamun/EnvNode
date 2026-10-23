# Kompatibilitätsübersicht

Stand: 12. Oktober 2026. Die Tabellen unterscheiden Anschlussmöglichkeit,
vorhandene Firmware-Unterstützung und tatsächlich bestätigten Betrieb.
**Ein passender Stecker oder eine erfolgreiche Identifikation allein bedeutet
keine vollständige Unterstützung.**

## Mindestversion richtig lesen

Die Firmware-Versionsnummer allein ist derzeit nicht eindeutig: viele Erweiterungen
wurden unter unveränderter **0.5.0** entwickelt. Deshalb nennt die Übersicht bei
solchen Funktionen zusätzlich den ersten relevanten Quellcommit. Ein Build mit
0.5.0 kann älter sein und die Funktion noch nicht enthalten. In der Firmware-Seite
Build-Identität und Git-Commit prüfen; der angegebene Commit muss im Build enthalten
sein (oder ein nachfolgender Stand, der die Funktion beibehält).

**0.5.0 Basis** bezeichnet den veröffentlichten Versionsstand ab `e449374`.
Ältere Sensorimplementierungen existierten bereits unter der damaligen 0.1.0;
die Tabelle nennt für den heutigen EnvNode-Einsatz bewusst die 0.5.0-Basis.
Ein Mindestquellstand ist kein Nachweis eines geprüften Binär-Releases oder
vollständiger Hardwarequalifikation.

## Mainboards und Firmware

| Mainboard | Hardwarestand | Firmware-Zuordnung | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| Mini 0.3 | Bestehender Aufbau | Boardprofil EnvNode Mainboard 0.3 | Nutzeraufbau bestätigt | 0.5.0 ab `d85e005` (Boardprofil); Beispielgerät 0.5.0+329 | DuoRelay 0.3, zwei SSD1309 und BME280 in Betrieb; SHTC3 konfiguriert, deaktiviert. |
| [Mini 0.8.1](mainboards/mini/index.md) | Aktuell dokumentierte Hardware | Boardprofil EnvNode Mini 0.8 | Nutzeraufbau von Mini 0.8 technisch und funktional bestätigt | Quellstand mit Profil `EnvNodeMini = 2` | Alle PCB-Patchstände 0.8.x verwenden Identity 0.8; Slot A GPIO33/GPIO13/GPIO5, Slot B GPIO32/GPIO14/GPIO27. |
| [Weather 0.2.1](mainboards/weatherstation/index.md) | Aktuell dokumentierte Hardware | Boardprofil EnvNode Weather 0.2 | Hier nicht bestätigt | Quellstand mit Profil `EnvNodeWeather = 1` | Ein realer Modulslot mit GPIO33/GPIO13/GPIO5. |
| [ORing Power 0.1.1](mainboards/oring-power/index.md) | Versorgungsbaustein | Kein Firmware-Treiber erforderlich | Elektrische Prüfung je Aufbau | Keine eigene Firmware erforderlich | Keine USB-Datenverbindung; Versorgung und Belastbarkeit nach Hardwareanleitung prüfen. |

### Nicht zu verwendende Hardwarestände

| Mainboard | Revisionen | Status | Kommentar |
| --- | --- | --- | --- |
| Weather | 0.1 | Nicht zu verwenden | Wegen fehlerhafter Slotkonfiguration vom Projektinhaber ausgeschlossen. |
| EnvNode Mini | 0.4–0.7 | Nicht zu verwenden | Diese Zwischenstände werden wegen der Slotkonfiguration nicht als unterstützte Aufbauten geführt. |

Diese Kennzeichnung betrifft die Hardwarestände. Ein neuer Firmwarestand allein
macht daraus keine bestätigte Kombination. Die Übersicht führt Mini 0.3 als
bestehenden bestätigten Aufbau und Mini 0.8.1 als aktuellen Dokumentationsstand.

Das historische Profil EnvNode Mainboard 0.3 beschreibt Slot A mit
AUX_GPIO1=GPIO4 und AUX_GPIO2=GPIO13, Slot B mit GPIO14 und GPIO16.
Mini 0.8 verwendet ein eigenes Profil mit Slot A GPIO33/GPIO13 und Slot B
GPIO32/GPIO14; Weather 0.2 verwendet Slot A GPIO33/GPIO13. Die Board Identity muss eine
unterstützte reale Belegung beschreiben; eine beliebige Revisionsangabe ändert
die Ressourcentabelle nicht automatisch.

## Erweiterungsmodule

| Modul | Hardware-Anschluss | Firmware-Unterstützung | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| [DuoRelay 0.3](modules/duo-relay/index.md) | EnvNode-Modulslot, 5 V und AUX_GPIO1/2 | On/Off; Descriptor-basierter Module-On/Off-Pfad vorhanden | Mini 0.3, Slot A bestätigt | GPIO: 0.5.0 Basis; Modulaktoren: ab `4cd8519` | Bei neueren Mainboards GPIO-Abgleich nötig. Identity-EEPROM U2 mit C1; erkannter Descriptor im Beispielaufbau vorhanden. |
| [AnalogHydroPressure 0.5](modules/analog-hydro-pressure/index.md) | Modulslot, ADC über I²C und Boost über AUX_GPIO2 | Kein registrierter ADS1115-Drucksensorpfad | Vollständiger Messbetrieb hier nicht bestätigt | Nicht verfügbar als vollständiger Messpfad | JP5/JP6 synchron auf I2C0 oder I2C1; genau einen Adressjumper setzen. Hardwareanleitung ist keine fertige Firmwareintegration. |

## Sensoren

| Sensor / Baugruppe | Anschluss | Firmware-Treiber | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| BME280 | I2C0 oder I2C1, 0x76/0x77 | Vorhanden | Mini 0.3, I2C1/0x76 bestätigt | 0.5.0 Basis | Temperatur, relative Feuchte und Luftdruck; Screenshot zeigt aktiven Sensor. |
| SHT4x | I2C0 oder I2C1, 0x44 | Vorhanden | Im hier gezeigten Aufbau nicht bestätigt | 0.5.0 Basis | Temperatur und relative Feuchte; Treiberunterstützung ist kein Nachweis jedes Sensorboards. |
| SHTC3 | I2C0 oder I2C1, 0x70 | Vorhanden | Konfiguriert, aber deaktiviert | 0.5.0 ab `91fb839` | Deaktivierter Eintrag erzeugt keine Messungen und belegt keinen aktuellen Funktionstest. |
| AM2302 / DHT22 | Geeigneter GPIO | Vorhanden | Hier nicht bestätigt | 0.5.0 Basis | Eigenes Einleitungsprotokoll, kein I²C; Temperatur und Feuchte. |
| Rain Gauge / Kippwaage | Geeigneter GPIO-Interrupt | Vorhanden | Hier nicht bestätigt | 0.5.0 Basis | Entprellung und Millimeter pro Kippung konfigurieren; nicht mit RainDetector verwechseln. |
| [RainDetectorController 0.3 + Head 0.1](sensors/rain-detector.md) | Frequenzeingang und Heiz-Steuerausgang | Separate PCNT-Laborfirmware; kein registrierter produktiver Sensor | WIP | Keine integrierte Mindestversion; separates Lab-Projekt | Mechanik, Heizung und Wasserausschluss noch zu prüfen; Laborfirmware ohne MQTT und Heizautomation. |
| [SCT013 / SCT013Head 0.1](sensors/sct013.md) | ADC, passende Variante mit Spannungsausgang | Kein registrierter SCT013-Treiber | WIP | Noch kein integrierter Treiber | [Aufbau und Erstprüfung](sensors/sct013-head/assembly.md) dokumentiert; reale Erprobung und Kalibrierung offen. Nur Spannungsausgang mit integrierter Bürde. |
| Simulierte Temperatur / Feuchte / Druck | Keine externe Hardware | Vorhanden | Im Beispielaufbau aktiv | 0.5.0 Basis | Prüft Softwareabläufe, keine physische Sensorfunktion. |

[Details zu I²C-Anschlüssen und Treibern](sensors/i2c.md)

## GeoPosition und abgeleiteter Druck

[GeoPosition und Meereshöhendruck](firmware/geo-position-pressure.md) sind ab
**Firmware 0.6.0** implementiert und auf einem Gerät geprüft. BME280 und simulierte
Barometer behalten den örtlichen Druck und bieten einen getrennten abgeleiteten
Wert. Voraussetzung für einen gültigen Meereshöhendruck ist die konfigurierte
Sensorhöhe. Eine lokale Temperaturquelle ist optional; ihre Auswahl und Aktualität
bestimmen Berechnungsmethode und Qualitätskennzeichnung. Diese Prüfung bestätigt
nicht pauschal jede Sensorplatine oder Installation.

## Displays

| Displaycontroller | Anschluss / Format | Firmware-Unterstützung | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| SSD1309 | I²C, 128×64, 0x3C/0x3D | Vorhanden | Zwei Displays am Mini 0.3 bestätigt | 0.5.0; für zwei Ausgänge ab `dd71d3f` | Im Beispiel je 0x3C auf I2C0 und I2C1; getrennte Busse vermeiden Adresskonflikt. |
| SSD1306 | I²C, 128×64, 0x3C/0x3D | Vorhanden | Hier nicht bestätigt | 0.5.0 ab `ae1c1aa`; zwei Ausgänge ab `dd71d3f` | Konkretes Modul, Versorgung und Logikpegel prüfen. |
| SH1106 | I²C, 128×64, 0x3C/0x3D | Vorhanden | Hier nicht bestätigt | 0.5.0 ab `ae1c1aa`; zwei Ausgänge ab `dd71d3f` | Kein Nachweis für SPI-Ausführungen oder andere Auflösungen. |

Es sind zwei Ausgänge konfigurierbar. Zwei Geräte mit gleicher Adresse benötigen
verschiedene Busse oder eine tatsächlich umstellbare Geräteadresse.
Die Summe aller angeschlossenen Pull-ups ist je Bus zu berücksichtigen.
[Displayeinrichtung](displays/index.md)

## Eine Kombination vor dem Betrieb prüfen

1. Reale Platinenrevision und aktives Boardprofil vergleichen.
2. Schnittstelle, Signalpegel, Versorgung und mechanische Belegung prüfen.
3. Busadressen und gemeinsam genutzte GPIOs auf Konflikte prüfen.
4. Passende Firmwareimplementierung auswählen und Änderungen anwenden.
5. Lokale Funktion prüfen, dann Controller und externe Integration einrichten.

Die Übersicht bewertet den dokumentierten Anschluss- und Softwarestand,
nicht eine Fertigungs-, Netzspannungs- oder Sicherheitsfreigabe.
Ein zusätzlicher bestätigter Aufbau wird mit Revision, Anschluss und Firmwarestand
aufgenommen; fehlende Bestätigung ist kein Nachweis einer Inkompatibilität.
