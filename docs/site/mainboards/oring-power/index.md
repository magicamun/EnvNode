# ORing Power

ORing Power versorgt das Mainboard mit 5 V und stellt die gemeinsame 3,3-V-Versorgung
über Regler und Dioden-ORing bereit. Das Modul hat keine eigene Firmware-Konfiguration.

## Was möchtest du tun?

| Dein Ausgangspunkt | Hier geht es weiter |
| --- | --- |
| Versorgungsmodul aufbauen und prüfen | [Aufbau und elektrische Prüfung, Revision 0.1.2](assembly.md) |
| Ein versorgtes Mainboard einrichten | [Erste Software-Inbetriebnahme](../../firmware/configuration.md) |
| Versorgung und Anschlüsse nachvollziehen | [Technische Anschlüsse und Prüfungen](assembly.md) |

USB-C am ORing dient der Stromversorgung. Für das Aufspielen der Firmware wird
der Datenanschluss des ESP32-DevKit verwendet. J1 und USB-C liegen parallel;
die Auswahl der 5-V-Quelle und die Rückspeiseprüfung beschreibt die Aufbauanleitung.

[Kompatibilität](../../compatibility.md) · [Alte Revisionen](old-revisions.md)
