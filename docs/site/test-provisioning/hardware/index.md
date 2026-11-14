# Testhardware

Die Prüfvorrichtung besteht aus Controller, Input-Adapter und passivem Modul-Adapter.
Die Aufbauanleitungen beschreiben Bestückung, Kontaktbelegung und elektrische Prüfung.

| Baugruppe | Aktuelle Revision | Technische Anleitung |
| --- | --- | --- |
| Test-Controller | 0.2 | [Aufbau und elektrische Prüfung](test-controller/assembly.md) |
| ESP32 DevKitC Adapter | 0.3 | [Aufbau und vollständige Kanalzuordnung](input-adapter/assembly.md) |
| Modul-Adapter | 0.2 | [Aufbau und 1:1-Kontaktprüfung](module-adapter/assembly.md) |

Der Legacy-Input-Adapter 0.1 bleibt mit seiner eigenen Kanalzuordnung unterstützt.
Die [Adapterauswahl](../commissioning/hardwareprofile.md) beschreibt
zulässige Kombinationen und den Stand der physischen Abnahme.

## Software-Inbetriebnahme

Nach der elektrischen Prüfung folgen [Tester einrichten](../commissioning/index.md),
[Selbsttests](../commissioning/selbsttests.md) und
[Mainboard prüfen](../operation/board-pruefung.md).

[Hardwareübersicht](../../hardware/index.md) · [Alte Revisionen](../../guides/old-revisions.md)
