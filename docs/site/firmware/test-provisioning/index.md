# Test/Provisioning

**Stand: Test-Firmware 0.9.2, Hardware-Inbetriebnahme.** Die Prüfanwendung verwendet
Netzwerk, Weboberfläche, Sprache/Zeit, Logs und OTA aus der gemeinsamen Basis,
führt aber keine EnvNode-Sensoren, Aktoren oder lokalen Controller aus.

## Aktueller Funktionsumfang

| Bereich | Stand |
| --- | --- |
| Hardwarediagnose `/diagnostics` | Reale manuelle Kanal-/Enable-Auswahl, LOW/HIGH und GPIO34-ADC |
| Hardware-Selbsttest `/selftest` | Geführte reale Schleifenprüfung von 22 Output- und 40 Input-Kontakten |
| Anschlussprüfung `/test` | Reale LOW/HIGH-Messung oder ausdrücklich gewählte Simulation; Boardbild, LEDs und Einzelmatrix |
| Prüfprofile | Vier eingebaute Profile; validierte Remote-Profile von docs.envnode.de mit lokalem Fallback |
| Board-Provisioning `/hardware` | Reales Schreiben und Rücklesen eines externen Board-EEPROMs über SDA16/SCL5 |
| Modul-Provisioning | In dieser Test-Firmware deaktiviert |
| Alternative DevKit-Adapter | Noch keine Adapterauswahl; nur der ursprüngliche Input-Adapter 0.1 wird angesteuert |

Die reale Anschlussprüfung ist seit 0.9.0 integriert. Die Grundansteuerung der
Multiplexer und der ADC-Rückweg wurden am aufgebauten Controller/Input-Adapter
mit Schleifenverbindungen bestätigt. **Die Abnahme des vollständigen Mainboardtests,
der Referenzdioden und des externen Provisionings am neuen Aufbau steht noch aus.**
ADC-Grenzen sind deshalb vorläufige Inbetriebnahmewerte.

## Passende Hardware

Unterstützt wird **Test-Controller 0.1 + ursprünglicher Input-Adapter 0.1**;
der passive Moduladapter ergänzt den 14-poligen Modulanschluss. Die drei
Input-Multiplexer stellen 38 ESP32-Sockelkontakte und zwei zusätzliche
Referenzkontakte bereit. Der erzeugte Fixture-Datensatz heißt
`controller0.1-input0.1-module0.1`.

Die neu angelegten Adapter **ESP32 DevKitC** und **ESP32 S3 DevKitC** haben andere
Zuordnungen und dürfen mit diesem festen Mapping noch nicht verwendet werden.
Die Bezeichnung „Input-Adapter“ in dieser Anleitung meint immer die produzierte
ursprüngliche Platine, auch wenn ihr KiCad-Verzeichnis inzwischen verschoben wurde.

## Ablauf

1. [Firmware und Versorgung einrichten](inbetriebnahme.md).
2. [Controller, Adapter und Pigtails mit Selbsttests prüfen](selbsttests.md).
3. [Ein Mainboard kontaktweise prüfen](board-pruefung.md).
4. Bei Bedarf [Boardidentität provisionieren](provisioning-profile.md).

Die Hardwareseiten erklären [Test-Controller](../../mainboards/test-controller/index.md),
[Input-Adapter](../../mainboards/input-adapter/index.md) und
[Moduladapter](../../mainboards/module-adapter/index.md). Die dortigen
Bestückungsanleitungen ersetzen nicht die elektrische Inbetriebnahme.
