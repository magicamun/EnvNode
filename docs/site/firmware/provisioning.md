# Hardware identifizieren und provisionieren

Provisioning schreibt die Identität einer Baugruppe in deren EEPROM.
Es ist bei leerem oder fehlerhaftem Datensatz nötig, nicht bei jedem Neustart.

| Speicher | Aufgabe |
| --- | --- |
| Mainboard-EEPROM | Board Identity und Auswahl des passenden Boardprofils |
| Modul-EEPROM | Module Identity beziehungsweise Descriptor und Kompatibilitätsprüfung |
| ESP32-NVS | WLAN, MQTT und Anwendungs-Konfiguration |

## Erkennung zuerst prüfen

Unter **Hardware** erkannte Identität und aktives Profil kontrollieren;
unter **Diagnose** gegebenenfalls I²C scannen. Ein nicht erreichbares EEPROM
zuerst auf Bestückung, Versorgung und Verbindung prüfen. Ein I²C-Antwortsignal
bestätigt noch keinen gültigen Datensatz.

| EEPROM | Bus | Adresse |
| --- | --- | --- |
| Mainboard | I2C0 | 0x50 |
| Modul Slot A | I2C0 | 0x52 |
| Modul Slot B, wenn vorhanden | I2C0 | 0x53 |

## Mainboard schreiben

1. Mainboard und reale PCB-Revision anhand der Platine feststellen.
2. Unter **Hardware** das passende angebotene Boardprofil auswählen.
3. Die Seriennummer als gültige Zahl eintragen.
4. Profil, zugehörige Revision und Seriennummer überprüfen.
5. Die ausdrückliche Schreibbestätigung setzen und den Vorgang ausführen.
6. Nur eine erfolgreiche Schreib- und Rückleseprüfung als Erfolg werten.
7. Neustarten und aktives Boardprofil erneut prüfen.

Die Revision wird durch das gewählte unterstützte Profil bestimmt. Keine
ähnliche oder höhere Version als Ersatz für ein fehlendes Profil wählen.
Das Schreiben ändert das aktive Boardprofil des laufenden Boots nicht;
der Neustart ist deshalb Bestandteil des Ablaufs.

## Modul schreiben

Die aktuelle Webfunktion bietet die Descriptor-Vorlage **DuoRelay 0.3**.
Slot A oder B passend zum eingesetzten Modul wählen, die angebotenen
Fertigungsangaben prüfen und die ausdrückliche Bestätigung verwenden.
Ergebnis und die anschließend erkannte Identität kontrollieren.
Andere Module werden nicht durch Auswahl der DuoRelay-Vorlage provisioniert.

Ein leeres EEPROM muss mit einem passenden Descriptor provisioniert
werden. Die hier dokumentierte Webfunktion bietet noch keine passende Vorlage
für AnalogHydroPressure; dafür niemals die DuoRelay-Vorlage verwenden.

## Was danach noch nötig ist

Identifikation erzeugt nicht automatisch eine fertige Anwendung.
Sensoren, Aktoren und Controller separat konfigurieren und ihre Änderungen anwenden.
Die ADS1115-Adresse am AnalogHydroPressure ist unabhängig von der Identity-Adresse;
dessen Buswahljumper betreffen den ADC, nicht das Identity-EEPROM.

[Mini](../mainboards/mini/index.md) · [Weather](../mainboards/weatherstation/index.md)
· [DuoRelay](../modules/duo-relay/index.md)
