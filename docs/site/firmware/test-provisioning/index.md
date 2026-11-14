# Test/Provisioning

**Stand: Test-Firmware 0.15.0, Hardware-Inbetriebnahme.** Die Prüfanwendung verwendet
Netzwerk, Weboberfläche, Sprache/Zeit, Logs und OTA aus der gemeinsamen Basis,
führt aber keine EnvNode-Sensoren, Aktoren oder lokalen Controller aus.

## Aktueller Funktionsumfang

| Bereich | Stand |
| --- | --- |
| Hardwarediagnose `/diagnostics` | Reale manuelle Kanal-/Enable-Auswahl, LOW/HIGH und GPIO34-ADC |
| Hardware-Selbsttest `/selftest` | Geführte reale Schleifenprüfung: 22 Output-Pfade bei Controller 0.1, 24 bei 0.2; jeweils 40 Input-Kontakte |
| Anschlussprüfung `/test` | Reale LOW/HIGH-Messung oder ausdrücklich gewählte Simulation; Boardbild, LEDs und Einzelmatrix |
| Prüfprofile | Sechs eingebaute Mainboardprofile; validierte Remote-Profile von docs.envnode.de mit lokalem Fallback |
| Board-Provisioning `/hardware` | Reales Schreiben und Rücklesen eines externen Board-EEPROMs über SDA16/SCL5 |
| Modul-Provisioning | Gemeinsamer Provisioning-Core, externe EEPROMs 0x52/0x53; reale Abnahme am Tester folgt |
| Hardwareprofile | Controller 0.1/0.2 und Legacy-/ESP32-DevKitC-Adapter getrennt auswählbar |

Die reale Anschlussprüfung ist seit 0.9.0 integriert. Die Grundansteuerung der
Multiplexer und der ADC-Rückweg wurden am aufgebauten Controller/Input-Adapter
mit Schleifenverbindungen bestätigt. Mainboardmessungen und gerichtete Pfade wurden
praktisch erprobt. Das externe Provisioning wurde an Weather 0.3 einschließlich Stromausfall erprobt.
**Die Abnahme der neuen Hardwareprofile steht noch aus.**
ADC-Grenzen sind deshalb vorläufige Inbetriebnahmewerte.

## Passende Hardware

Unterstützt werden Controller 0.1 und 0.2 sowie Input-Adapter 0.1 (Legacy) und
ESP32 DevKitC Adapter 0.3. Die Auswahl gilt für reale Messung, Selbsttest und
Protokollierung und wird gespeichert. Details unter
[Controller und Adapter auswählen](hardwareprofile.md).

Der passive Moduladapter ergänzt den 14-poligen Modulanschluss. Alle derzeitigen
Adapterprofile stellen 38 Socketkontakte und zwei Referenzkontakte bereit.
ESP32-S3 mit 44 Kontakten folgt später.

## Ablauf

1. [Firmware und Versorgung einrichten](inbetriebnahme.md).
2. [Controller, Adapter und Pigtails mit Selbsttests prüfen](selbsttests.md).
3. [Ein Mainboard kontaktweise prüfen](board-pruefung.md).
4. Bei Bedarf [Boardidentität provisionieren](provisioning-profile.md).

Die Hardwareseiten erklären [Test-Controller](../../mainboards/test-controller/index.md),
[Input-Adapter](../../mainboards/input-adapter/index.md) und
[Moduladapter](../../mainboards/module-adapter/index.md). Die dortigen
Bestückungsanleitungen ersetzen nicht die elektrische Inbetriebnahme.
