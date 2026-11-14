# Kompatibilitätsübersicht

Stand: 14. November 2026. Die Tabellen unterscheiden Anschlussmöglichkeit,
vorhandene Firmware-Unterstützung und tatsächlich bestätigten Betrieb.
**Ein passender Stecker oder eine erfolgreiche Identifikation allein bedeutet
keine vollständige Unterstützung.**

## Mindestversion richtig lesen

Die Tabellen nennen die normale **EnvNode-Firmware**. Die separate Test-Firmware
hat eine eigene Versionsreihe. Für Mini 0.10 und Weather 0.4 sind beispielsweise
EnvNode **0.8.11** und Tester **0.15.0** dokumentiert.

Bei Mainboards ist der Mindeststand des **aktuellen Hardwareprofilkatalogs**
angegeben. Ältere Aufbauten wurden bereits mit früheren Firmwareständen betrieben;
dieser historische Betriebsnachweis ist getrennt ausgewiesen. Die Profilangaben
stammen aus `firmware/hardware-profiles/` und bestätigen keine physische Abnahme.

Für eine Kombination muss die Firmware sowohl das Boardprofil als auch die
gewünschte Sensor-, Aktor- oder Controllerfunktion unterstützen. Maßgeblich ist
der höhere Mindeststand. Eine bestätigte Treiberfunktion ersetzt nicht die
Prüfung des konkreten Sensorboards oder der tatsächlichen Verdrahtung.

Bei älteren Funktionen mit der Angabe **0.5.0** ergänzen Quellcommits den
Mindeststand, weil damals mehrere Erweiterungen unter derselben Versionsnummer
entwickelt wurden. **0.5.0 Basis** bezeichnet den Stand ab `e449374`.

## Mainboards und Firmware

| Mainboard | Hardwarestand | Firmware-Zuordnung | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| [Mini 0.10.0](mainboards/mini/revisions/0.10.0.md) | Aktueller CAD-Stand | EnvNode Mini, Identity 0.10 | Physische Abnahme ausstehend | 0.8.11 | Zwei Modulslots; Resetanschluss RST1. Tester ab 0.15.0. |
| [Weather 0.4](mainboards/weatherstation/revisions/0.4.md) | Aktueller CAD-Stand | EnvNode Weather, Identity 0.4 | Physische Abnahme ausstehend | 0.8.11 | Ein Modulslot J5; Resetanschluss RST1. Tester ab 0.15.0. |
| [Mini 0.9.0](mainboards/mini/assembly.md) | Vollständige Aufbauanleitung vorhanden | EnvNode Mini, Identity 0.9 | Elektrische Prüfung je Aufbau; kein zusätzlicher Betriebsnachweis hier | 0.8.2 | Steckbare Pull-ups und lokale EEPROM-Abblockung; zwei Modulslots. |
| [Weather 0.3.0](mainboards/weatherstation/assembly.md) | Vollständige Aufbauanleitung vorhanden | EnvNode Weather, Identity 0.3 | Prüf-/Provisioningbetrieb dokumentiert; kein pauschaler Nachweis jeder Sensorbestückung | 0.8.2 | Ein Modulslot; steckbare Pull-ups und lokale EEPROM-Abblockung. |
| Mini 0.3 | Bestehender historischer Aufbau | EnvNode Mainboard, Identity 0.3 | Nutzeraufbau mit Firmware 0.5.0+329 bestätigt | 0.8.2 für den aktuellen Profilkatalog | Im damaligen Aufbau DuoRelay 0.3, zwei SSD1309 und BME280; SHTC3 deaktiviert. |
| [Mini 0.8.1](mainboards/mini/revisions/0.8.1.md) | Historischer dokumentierter Stand | EnvNode Mini, Identity 0.8 | Nutzeraufbau von Mini 0.8 technisch und funktional bestätigt | 0.8.2 für den aktuellen Profilkatalog | Alle PCB-Patchstände 0.8.x verwenden Identity 0.8. |
| [Weather 0.2.1](mainboards/weatherstation/revisions/0.2.1.md) | Historischer dokumentierter Stand | EnvNode Weather, Identity 0.2 | Hier nicht bestätigt | 0.8.2 für den aktuellen Profilkatalog | Ein realer Modulslot. |
| [ORing Power 0.1.2](mainboards/oring-power/index.md) | Versorgungsbaustein | Kein Firmware-Treiber erforderlich | Elektrische Prüfung je Aufbau | Keine eigene Firmware erforderlich | USB-C liefert nur Strom; 5-V-Eingänge sind parallel, nicht gegeneinander entkoppelt. |

### Nicht zu verwendende Hardwarestände

| Mainboard | Revisionen | Status | Kommentar |
| --- | --- | --- | --- |
| Weather | 0.1 | Nicht zu verwenden | Wegen fehlerhafter Slotkonfiguration vom Projektinhaber ausgeschlossen. |
| EnvNode Mini | 0.4–0.7 | Nicht zu verwenden | Diese Zwischenstände werden wegen der Slotkonfiguration nicht als unterstützte Aufbauten geführt. |

Ein neuer Firmwarestand allein korrigiert die ausgeschlossenen Hardwarestände
nicht. Für jede unterstützte Revision müssen Board Identity und tatsächliche
Belegung zusammenpassen. Die [HW-Schnittstellen](interfaces/index.md) und die
jeweilige Aufbauanleitung beschreiben die Anschlüsse; die GPIO-Zuordnung wird
hier nicht erneut als zweite Pinout-Tabelle geführt.

## Erweiterungsmodule

| Modul | Hardware-Anschluss | Firmware-Unterstützung | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| [DuoRelay 0.3](modules/duo-relay/index.md) | EnvNode-Modulslot, 5 V und AUX_GPIO1/2 | On/Off; Descriptor-basierter Module-On/Off-Pfad vorhanden | Mini 0.3, Slot A bestätigt | GPIO: 0.5.0 Basis; Modulaktoren: ab `4cd8519` | Bei neueren Mainboards GPIO-Abgleich nötig. Identity-EEPROM U2 mit C1; erkannter Descriptor im Beispielaufbau vorhanden. |
| [AnalogHydroPressure 0.6](modules/analog-hydro-pressure/index.md) | Modulslot, ADS1115 über I²C und Boost über AUX_GPIO2 | Kein registrierter ADS1115-Drucksensorpfad | Vollständiger Messbetrieb hier nicht bestätigt | Nicht verfügbar als vollständiger Messpfad | J1/J2 gemeinsam auf denselben Bus setzen; genau ein Adresspaar an J6 brücken. Ein EEPROM-Descriptor aktiviert weder ADC noch Boost. |
| [I²C Carrier HalfSize 0.1](modules/i2c-carrier/index.md) | Passive Trägerplatine für I²C-Sensorboards | Kein eigener Treiber erforderlich | Anschlussprüfung je Aufbau | Mindeststand des aufgesetzten Sensors | Buswahl und Adressen müssen zum tatsächlichen Sensor passen. |

## Sensoren

| Sensor / Baugruppe | Anschluss | Firmware-Treiber | Betriebsnachweis | Mindeststand Firmware | Kommentar |
| --- | --- | --- | --- | --- | --- |
| BME280 | I2C0 oder I2C1, 0x76/0x77 | Vorhanden | Mini 0.3, I2C1/0x76 bestätigt | 0.5.0 Basis | Temperatur, relative Feuchte und Luftdruck; Screenshot zeigt aktiven Sensor. |
| SHT4x | I2C0 oder I2C1, 0x44 | Vorhanden | Im hier gezeigten Aufbau nicht bestätigt | 0.5.0 Basis | Temperatur und relative Feuchte; Treiberunterstützung ist kein Nachweis jedes Sensorboards. |
| SHTC3 | I2C0 oder I2C1, 0x70 | Vorhanden | Konfiguriert, aber deaktiviert | 0.5.0 ab `91fb839` | Deaktivierter Eintrag erzeugt keine Messungen und belegt keinen aktuellen Funktionstest. |
| AM2302 / DHT22 | Geeigneter GPIO | Vorhanden | Hier nicht bestätigt | 0.5.0 Basis | Eigenes Einleitungsprotokoll, kein I²C; Temperatur und Feuchte. |
| Rain Gauge / Kippwaage | Geeigneter GPIO-Interrupt | Vorhanden | Hier nicht bestätigt | 0.5.0 Basis | Entprellung und Millimeter pro Kippung konfigurieren; nicht mit RainDetector verwechseln. |
| [RainDetectorController 0.4 + Head 0.2](sensors/rain-detector/index.md) | Frequenzeingang und Heiz-Steuerausgang | RainDetector (frequency) und lokaler RainDetector-Controller integriert | Benetzung, Regenende und Trocknung am Versuchsaufbau geprüft; Feld-/Mechanikabnahme je Aufbau | 0.8.12 für den dokumentierten integrierten Betrieb | Schwellen je Head einmessen. Optionales Taupunktheizen ist implementiert; Praxistest mit Klimaquelle noch ausstehend. [Inbetriebnahme](commissioning/rain-detector.md). |
| [SCT013 / SCT013Head 0.2](sensors/sct013/index.md) | Freier ADC1-Analogport, Spannungsausgang mit integrierter Bürde | SCT013 AC RMS mit DMA-Erfassung, Kalibrierung, Filter und Nullpunkthysterese | DMA-Betrieb am Mini erprobt; Dauertest und Fehlerwiederanlauf weiter beobachten | 0.8.14 | Genau ein aktiver SCT013-Slot; RainDetector kann parallel laufen. Rev. 0.1 benötigt die Buchsenkorrektur. Keine Leistungs- oder Energiemessung. |
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
