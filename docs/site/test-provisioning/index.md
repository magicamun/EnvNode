# Test/Provisioning

Mit dem EnvNode-Tester prüfst du die elektrischen Verbindungen eines Mainboards
und schreibst bei Bedarf Board- oder Modulidentitäten in dessen EEPROM.
Dafür gibt es eine eigene Prüfvorrichtung und die separate Test-Firmware.

## Wo möchtest du einsteigen?

| Dein Vorhaben | Hier geht es weiter |
| --- | --- |
| Prüfvorrichtung selbst aufbauen | [Hardware und Aufbau](hardware/index.md) |
| Einen elektrisch geprüften Aufbau erstmals verwenden | [Firmware und Versorgung einrichten](commissioning/index.md) |
| Controller und Adapter zuordnen | [Hardwareprofile auswählen](commissioning/hardwareprofile.md) |
| Verkabelung und Messpfade des Testers kontrollieren | [Hardware-Selbsttests](commissioning/selbsttests.md) |
| Ein Mainboard kontaktweise prüfen | [Mainboards prüfen](operation/board-pruefung.md) |
| Board- oder Modulidentität schreiben | [Provisioning und Profile](operation/provisioning-profile.md) |
| Fertige Test-Firmware herunterladen | [Tester-Downloads](../firmware/downloads.md#tester) |

## Hardware und Arbeitsabläufe

Die Prüfvorrichtung besteht aus Test-Controller, Input-Adapter und passivem
Modul-Adapter. Die technischen Anleitungen enthalten Bestückung, Pinbelegung und
elektrische Prüfungen. Die Inbetriebnahme verbindet den geprüften Aufbau mit der
Software und führt durch Auswahl und Selbsttests.

Bei der **Anschlussprüfung** ist der Prüfling ohne Versorgung und ohne eingesetztes
ESP32-DevKit angeschlossen. **Provisioning** nutzt dagegen den separaten I²C-Bus
und kann den Prüfling versorgen. Verkabelung und Arbeitsphase müssen jeweils
zur Anleitung passen; der Wechsel wird unter [Provisioning](operation/provisioning-profile.md) erklärt.

Die reale Messung und die ausdrücklich wählbare Simulation sind in der Bedienung
getrennt ausgewiesen. Die jeweils unterstützten Hardwarekombinationen und deren
Abnahmestand stehen unter [Controller und Adapter auswählen](commissioning/hardwareprofile.md).

Du möchtest Sensoren betreiben und Messwerte erfassen? Dafür beginnt der Weg bei
[EnvNode: Dein erstes Gerät](../getting-started/index.md).
