# EnvNode PCB-Farbkonvention

Die Farbe der Lötstoppmaske kennzeichnet die Funktion einer EnvNode-Platine.
Die Zuordnung gilt auch für Prototypen und neue Revisionen.

| Kategorie | Farbe | HEX | RGB | Platinen |
| --- | --- | --- | --- | --- |
| Mainboards und Spannungsversorgung | Grün | `#16834A` | 22, 131, 74 | EnvNode Mini, WeatherStation, ORing Power |
| Funktionsmodule | Schwarz | `#141414` | 20, 20, 20 | DuoRelay, AnalogHydroPressure, SX1262-Remote |
| Sensor-Heads | Weiß | `#F2F2F2` | 242, 242, 242 | RainDetectorHead, SCT013, SolarRadiation |
| Sensor-Controller | Lila | `#8426A8` | 132, 38, 168 | RainDetectorController |
| Testhardware | Blau | `#1555A0` | 21, 85, 160 | Testboards, Debug-Adapter, Breakouts |
| Reserve | Gelb | `#E6BE00` | 230, 190, 0 | |
| Reserve | Rot | `#C62828` | 198, 40, 40 | |

Ein Sensor-Head mit integrierter Auswertung oder Heizungssteuerung bleibt weiß.
Lila kennzeichnet eine eigenständige Controllerplatine. Beim
RainDetectorController wurde Lila wegen der besseren Lesbarkeit der
Beschriftung in der JLCPCB-Ansicht gewählt.

## Darstellung in KiCad

Im PCB-Editor unter **Board Setup → Board Stackup → Physical Stackup**
für die obere und untere Lötstoppmaske denselben RGB-Wert aus der Tabelle
einstellen. KiCad speichert beide Farben in der `.kicad_pcb`. Im 3D-Viewer
unter **Appearance → Items** die Option **Use board stackup colors** aktivieren.

Die HEX- und RGB-Werte sind visuelle
Annäherungen an die JLCPCB-Lötstoppmaskenfarben. Für die Fertigungsbestellung
gilt der jeweilige Farbname.

Lizenz siehe [LICENSE](../../LICENSE).
