# Bestückungspläne

Die folgenden senkrechten Vorderseitenpläne werden in den Aufbauanleitungen verwendet. Die farbigen KiCad-Renderings zeigen die Leiterplatten ohne Bauteilmodelle; PNG eignet sich als Bildvorlage. Die Dateien sind nach Board und Revision getrennt.

| Board / Revision | Vorderseite PNG |
| --- | --- |
| [mini-0.9.0](../mainboards/mini/index.md) | [PNG](../assets/board-views/mini-0.9.0/assembly-front.png) |
| [weatherstation-0.3.0](../mainboards/weatherstation/index.md) | [PNG](../assets/board-views/weatherstation-0.3.0/assembly-front.png) |
| [analog-0.6](../modules/analog-hydro-pressure/index.md) | [PNG](../assets/board-views/analog-0.6/assembly-front.png) |
| [sd-card-0.1](../modules/sd-card/index.md) | [PNG](../assets/board-views/sd-card-0.1/assembly-front.png) |
| [rain-controller-0.4](../sensors/rain-detector-controller/assembly.md) | [PNG](../assets/board-views/rain-controller-0.4/assembly-front.png) |
| [rain-head-0.2](../sensors/rain-detector-head/assembly.md) | [PNG](../assets/board-views/rain-head-0.2/assembly-front.png) |
| [test-controller-0.1](../mainboards/test-controller/index.md) | [PNG](../assets/board-views/test-controller-0.1/assembly-front.png) |
| [input-adapter-0.1](../mainboards/input-adapter/index.md) | [PNG](../assets/board-views/input-adapter-0.1/assembly-front.png) |
| [module-adapter-0.1](../mainboards/module-adapter/index.md) | [PNG](../assets/board-views/module-adapter-0.1/assembly-front.png) |
| [duo-relay-0.3](../modules/duo-relay/index.md) | [PNG](../assets/board-views/duo-relay-0.3/assembly-front.png) |
| [oring-power-0.1.1](../mainboards/oring-power/index.md) | [PNG](../assets/board-views/oring-power-0.1.1/assembly-front.png) |
| [i2c-carrier-0.1](../modules/i2c-carrier/assembly.md) | [PNG](../assets/board-views/i2c-carrier-0.1/assembly-front.png) |
| [sct013-head-0.1](../sensors/sct013-head/assembly.md) | [PNG](../assets/board-views/sct013-head-0.1/assembly-front.png) |

## Für die Testsoftware

Die Vorderseitenbilder von Mini 0.9.0 und Weatherstation 0.3.0 sind für die Wiederverwendung in der Testsoftware vorgesehen. Die Ablage enthält außerdem die historischen Pläne für Mini 0.8.1 und Weather 0.2.1; Revisionen nicht mischen.

Der [Dateiindex](../assets/board-views/index.json) nennt die revisionsgebundenen Dateien. `views.json` je Board enthält PCB-Prüfsumme, PNG-Abmessungen, Render-Einstellungen und Bauteilpositionen in KiCad-Millimetern. Die Bilddateien enthalten keine Oberflächenbedienelemente oder Testmarkierungen. Für Positionsmarkierungen in der Testsoftware muss die Zuordnung der PCB-Koordinaten zu den Pixeln des zugeschnittenen Renderings kalibriert werden. Rückseitenansichten werden von B.Cu betrachtet.

[Aufbauanleitungen](index.md) · [Alte Revisionen](old-revisions.md)
