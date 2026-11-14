# Testhardware

Die Prüfvorrichtung besteht aus Controller, Input-Adapter und passivem Modul-Adapter.
Die Aufbauanleitungen beschreiben Bestückung, Kontaktbelegung und elektrische Prüfung.

| Baugruppe | Aktuelle Revision | Technische Anleitung |
| --- | --- | --- |
| Test-Controller | 0.2 | [Aufbau und elektrische Prüfung](../../mainboards/test-controller/assembly.md) |
| ESP32 DevKitC Adapter | 0.3 | [Aufbau und vollständige Kanalzuordnung](../../mainboards/input-adapter/assembly.md) |
| Modul-Adapter | 0.2 | [Aufbau und 1:1-Kontaktprüfung](../../mainboards/module-adapter/assembly.md) |

Der Legacy-Input-Adapter 0.1 bleibt mit seiner eigenen Kanalzuordnung unterstützt.
Die [Adapterauswahl](../../firmware/test-provisioning/hardwareprofile.md) beschreibt
zulässige Kombinationen und den Stand der physischen Abnahme.

## Software-Inbetriebnahme

Nach der elektrischen Prüfung folgen [Tester einrichten](../../firmware/test-provisioning/inbetriebnahme.md),
[Selbsttests](../../firmware/test-provisioning/selbsttests.md) und
[Mainboard prüfen](../../firmware/test-provisioning/board-pruefung.md).

[Hardwareübersicht](../index.md) · [Alte Revisionen](../../guides/old-revisions.md)
