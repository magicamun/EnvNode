# Bestückungspläne

Die folgenden senkrechten Vorderseitenpläne werden in den Aufbauanleitungen verwendet. Die farbigen KiCad-Renderings zeigen die Leiterplatten ohne Bauteilmodelle; PNG eignet sich als Bildvorlage. Die Dateien sind nach Board und Revision getrennt.

| Board / Revision | Vorderseite PNG |
| --- | --- |
| [Mini 0.10.0](../mainboards/mini/revisions/0.10.0.md) | [PNG](../assets/board-views/mini-0.10.0/assembly-front.png) |
| [Weather 0.4](../mainboards/weatherstation/revisions/0.4.md) | [PNG](../assets/board-views/weatherstation-0.4/assembly-front.png) |
| [mini-0.9.0](../mainboards/mini/revisions/0.9.0.md) | [PNG](../assets/board-views/mini-0.9.0/assembly-front.png) |
| [weatherstation-0.3.0](../mainboards/weatherstation/revisions/0.3.0.md) | [PNG](../assets/board-views/weatherstation-0.3.0/assembly-front.png) |
| [analog-0.6](../modules/analog-hydro-pressure/assembly.md) | [PNG](../assets/board-views/analog-0.6/assembly-front.png) |
| [rain-controller-0.4](../sensors/rain-detector-controller/assembly.md) | [PNG](../assets/board-views/rain-controller-0.4/assembly-front.png) |
| [rain-head-0.2](../sensors/rain-detector-head/assembly.md) | [PNG](../assets/board-views/rain-head-0.2/assembly-front.png) |
| [Test-Controller 0.2](../mainboards/test-controller/assembly.md) | [PNG](../assets/board-views/test-controller-0.2/assembly-front.png) |
| [ESP32 DevKitC Adapter 0.3](../mainboards/input-adapter/assembly.md) | [PNG](../assets/board-views/input-adapter-0.3/assembly-front.png) |
| [Modul-Adapter 0.2](../mainboards/module-adapter/assembly.md) | [PNG](../assets/board-views/module-adapter-0.2/assembly-front.png) |
| [test-controller-0.1](../mainboards/test-controller/revisions/0.1.md) | [PNG](../assets/board-views/test-controller-0.1/assembly-front.png) |
| [input-adapter-0.1](../mainboards/input-adapter/revisions/0.1.md) | [PNG](../assets/board-views/input-adapter-0.1/assembly-front.png) |
| [module-adapter-0.1](../mainboards/module-adapter/revisions/0.1.md) | [PNG](../assets/board-views/module-adapter-0.1/assembly-front.png) |
| [duo-relay-0.3](../modules/duo-relay/assembly.md) | [PNG](../assets/board-views/duo-relay-0.3/assembly-front.png) |
| [oring-power-0.1.1](../mainboards/oring-power/assembly.md) | [PNG](../assets/board-views/oring-power-0.1.1/assembly-front.png) |
| [i2c-carrier-0.1](../modules/i2c-carrier/assembly.md) | [PNG](../assets/board-views/i2c-carrier-0.1/assembly-front.png) |
| [SCT013Head: historische Ansicht 0.1](../sensors/sct013-head/assembly.md) | [PNG](../assets/board-views/sct013-head-0.1/assembly-front.png) |

Die SCT013Head-Pläne zeigen Rev. 0.1. Die aktuelle Rev. 0.2 korrigiert die
Lumberg-Buchsenverbindung; diese Leiterbahnkorrektur ist in den alten Bildern nicht enthalten.

## Für die Testsoftware

Die Vorderseitenbilder von Mini 0.9.0 und Weatherstation 0.3.0 sind für die Wiederverwendung in der Testsoftware vorgesehen. Die Ablage enthält außerdem die historischen Pläne für Mini 0.8.1 und Weather 0.2.1; Revisionen nicht mischen.

Der [Dateiindex](../assets/board-views/index.json) nennt die revisionsgebundenen Dateien. `views.json` je Board enthält PCB-Prüfsumme, PNG-Abmessungen, Render-Einstellungen und Bauteilpositionen in KiCad-Millimetern. Die Bilddateien enthalten keine Oberflächenbedienelemente oder Testmarkierungen. Für Positionsmarkierungen in der Testsoftware muss die Zuordnung der PCB-Koordinaten zu den Pixeln des zugeschnittenen Renderings kalibriert werden. Rückseitenansichten werden von B.Cu betrachtet.

[Aufbauanleitungen](index.md) · [Alte Revisionen](old-revisions.md)
