# RainDetector in Betrieb nehmen

Ziel: Ein elektrisch geprüfter RainDetectorController mit Head liefert eine
Frequenzmessung; ein lokaler Controller erkennt Benetzung und steuert das
Trockenheizen. Die gemeinsame Firmware unterstützt diesen Pfad ab **0.8.12**.
Die Laborprüfung ersetzt keine Abnahme der realen Mechanik, Beschichtung und
thermischen Eigenschaften des eigenen Aufbaus.

## Voraussetzungen und Anschluss

[Controller](../sensors/rain-detector-controller/assembly.md) und
[Head](../sensors/rain-detector-head/assembly.md) müssen elektrisch geprüft sein.
Dort stehen Kontaktbelegung, Versorgung, Heizlast und separate Funktionstests.
Benötigt werden ein geeigneter Frequenzeingang für RAIN_FREQ und ein freier
Digitalausgang für HEATER_EN. Wähle die Ressourcen nach tatsächlicher Verdrahtung
und aktivem Boardprofil. Die 5-V-Versorgung muss die Heizlast mittragen.

## Drei Instanzen einrichten

| Bereich | Auswahl | Zuordnung |
| --- | --- | --- |
| Sensoren | RainDetector (frequency) | Frequenzeingang RAIN_FREQ |
| Aktoren | GPIO PWM oder GPIO On/Off | Ausgang HEATER_EN, HIGH schaltet die Heizung ein |
| Controller | RainDetector | Den angelegten Frequenzsensor und Heizaktor auswählen |

Zunächst Sensor und Aktor einrichten, aktivieren, speichern und anwenden.
Den Frequenzwert mit dem zuvor elektrisch geprüften Aufbau vergleichen.
Danach den RainDetector-Controller zuordnen, aktivieren, speichern und anwenden.
Jeder Ausgang darf nur einen aktiven steuernden Controller haben.
Die gemeinsamen Formulare erklären [Sensoren](../firmware/sensors.md),
[Aktoren](../firmware/actuators.md) und [Controller](../firmware/controllers.md).

## Am eigenen Head einmessen

Die trockene und benetzte Frequenz am eigenen Aufbau erfassen. Daraus die
Trockenschwelle und Hysterese festlegen; Voreinstellungen sind keine Kalibrierung.
Die absolute und relative Frequenzabfallschwelle müssen beide erfüllt sein,
damit neue Benetzung erkannt wird.

| Parameter | Voreinstellung | Bedeutung |
| --- | --- | --- |
| Flankenfenster | 1000 ms | Vergleichsfenster für neue Benetzung |
| Absoluter Frequenzabfall | 150 Hz | Mindestabfall der Frequenz |
| Relativer Frequenzabfall | 5 ‰ | Zusätzlicher relativer Mindestabfall |
| Regenende-Ruhezeit | 60 s | Zeit ohne neu erkannte Benetzung |
| Trockenschwelle | 32000 Hz | Am eigenen Head überprüfen und einmessen |
| Trockenhysterese | 500 Hz | Band unterhalb der Trockenschwelle |
| Trockenbestätigung | 10 s | Erforderliche Bestätigungsdauer |

Das Sensor-Ausgabeintervall (voreingestellt 5000 ms) bestimmt die Veröffentlichung.
Die lokale Frequenzmessung arbeitet unabhängig davon in ungefähr 250-ms-Fenstern.
Die Automatik verwendet Heizung aus oder 100 %, auch bei einem PWM-Aktor.

## Funktion und Abschaltung prüfen

Aktivierte Sensoren und Controller starten nach Boot oder Laufzeitübernahme
automatisch. Die Heizung beginnt aus. Vor erster Benetzung mindestens sechs
Sekunden warten. Nach gültigem Signal und Wartezeit prüft die Automatik einmalig
den Ausgangszustand: unter der Trockenschwelle heizen, darüber trocken bestätigen.
Dieser Startvorgang erzeugt keine Regenmeldung.

Danach kontrolliert benetzen: neue Benetzung muss erkannt werden und Heizung
anfordern. Nach der Ruhezeit wird Regenende geschätzt; die Heizung läuft bis zur
Trockenbestätigung weiter. Beobachte dabei Stromaufnahme und reale Temperatur.
Regenende und trocken sind verschiedene Zustände.

**Stop** muss die Heizung ausschalten. Eine manuelle Aktoränderung entwaffnet
die Automatik; für erneuten Automatikbetrieb ist **Start** erforderlich.
Ungültige oder fehlende Frequenz muss die Heizung abschalten und den Regenzustand
auf unbekannt setzen. Signalwiederkehr allein ist nach verbrauchter Startprüfung
kein automatischer neuer Heizauftrag; danach ist neue Benetzung erforderlich.
Verdrahtungsänderungen für Prüfungen nur spannungsfrei vornehmen.

Nach Neustart Zuordnungen und Parameter kontrollieren. Für dauerhaft abgeschaltete
Automatik den Controller deaktivieren und speichern; Stop allein ist keine
persistente Deaktivierung. [Konfiguration sichern](../firmware/maintenance.md).

[Benetzung beobachten und Zustände verwenden](../applications/rain-detector.md)
