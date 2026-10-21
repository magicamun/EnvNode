# Hardware-Schnittstellen

Hier findest du die gemeinsamen Anschlussregeln und Pinbelegungen der
EnvNode-Mainboards. Die Produktseiten ergänzen die tatsächlichen Anschlussnamen,
Anzahl, Einbaulage und GPIO-Zuordnung des jeweiligen Boards.

| Schnittstelle | Anschluss | Verwendung |
| --- | --- | --- |
| [I²C](#i2c-jst-sh-vierpolig) | JST-SH, 4-polig, 1,00 mm | Digitale Sensoren und Displays |
| [ADC / GPIO](#adc-und-gpio-jst-ph-vierpolig) | JST-PH, 4-polig, 2,00 mm | Analoge Eingangssignale und geeignete digitale Signale |
| [Modulschnittstelle](../concepts/module-interface.md) | 2×7, 2,54 mm | Versorgung, zwei I²C-Busse, SPI und slotabhängige GPIOs |

## Bilder richtig lesen

Die Renderings zeigen die Mainboard-Bestückungsseite, nicht die Kontaktansicht
eines Kabelsteckers oder die Unterseite eines Moduls. Pin 1 ist als GND markiert.
Referenzabbildungen stammen vom Mini; ihre Pinfolge gilt für die entsprechenden
Anschlusstypen auf Mini und Weather. Drehrichtung, Anschlussname und Signalzuordnung
immer an der realen Platine kontrollieren. Virtuelle Schnittkanten stellen keine
vollständige Platinenkontur dar.

## I²C: JST-SH, vierpolig

![JST-SH: Schrägansicht mit Pin 1](../assets/images/connectors/J5-oblique-annotated.png){ width="48%" }
![JST-SH: Draufsicht](../assets/images/connectors/J5-top-annotated.png){ width="48%" }

| Pin | Signal | Kommentar |
| --- | --- | --- |
| 1 | GND | Gemeinsame Masse |
| 2 | +3V3_SYS | Systemversorgung für geeignete 3,3-V-Geräte |
| 3 | SDA | Datenleitung des ausgewählten Busses |
| 4 | SCL | Taktleitung des ausgewählten Busses |

Die seitlichen Metalllaschen sind mechanische Befestigungen, keine Signalpins.
Die Referenz J5 zeigt I2C0 mit SDA0/SCL0. Für I2C1 gilt dieselbe Pinfolge.
Mehrere Stecker desselben Busses sind parallele Anschlüsse, keine voneinander
isolierten Schnittstellen. Adressen und Pull-ups für alle angeschlossenen Geräte
gemeinsam planen. Zwei gleiche Adressen sind auf verschiedenen Bussen möglich.

[Unterstützte Sensoren und Buskonfiguration](../sensors/i2c.md)

## ADC und GPIO: JST-PH, vierpolig

![JST-PH: Schrägansicht mit Pin 1](../assets/images/connectors/ADC1-oblique-annotated.png){ width="48%" }
![JST-PH: Draufsicht](../assets/images/connectors/ADC1-top-annotated.png){ width="48%" }

| Pin | Signal | Kommentar |
| --- | --- | --- |
| 1 | GND | Gemeinsame Masse |
| 2 | +5V | Sensorversorgung; kein zulässiger GPIO-Signalpegel |
| 3 | Signal | Anschlussabhängiger ADC-Eingang oder GPIO |
| 4 | +3V3_SYS | Alternative Sensorversorgung |

Die Referenz zeigt ADC1 am Mini. Der Silkscreen `GPIO` bezeichnet dort das Signal
an Pin 3. Welche Fähigkeiten verfügbar sind, hängt vom zugeordneten ESP32-Pin ab.
Ein reiner Eingang darf nicht als Ausgang verwendet werden. ESP32-Signalpins sind
nicht 5-V-tolerant; Versorgungsspannung und Signalspannung getrennt beurteilen.
5 V und 3,3 V dürfen nicht miteinander verbunden werden.

Der zweipolige JST-PH-Eingang von ORing Power gehört nicht zu dieser vierpoligen
Pinbelegung. [Versorgung mit ORing Power](../mainboards/oring-power/index.md)

## Modulconnector: 2×7 Kontakte

![Modulbuchse mit markiertem Pin 1](../assets/images/module-interface/module-oblique-annotated.png)

![Modulschnittstelle: vollständige Pinbelegung](../assets/images/module-interface/module-pinout.png)

Die Buchse führt GND, 3,3 V, 5 V, EEPROM_A0, I2C0, I2C1, zwei AUX-GPIOs und SPI.
Die Busse werden gemeinsam verwendet; AUX_GPIO1, AUX_GPIO2 und SPI_CS sind
slotabhängig. Die Schnittstelle führt keine 24 V.

[Ausführliche Modulschnittstelle: alle 14 Pins, Identifikation und Mechanik](../concepts/module-interface.md)

## Boardbezogene Zuordnung

- [Mini: Stecker und GPIO-Zuordnung](../mainboards/mini/index.md)
- [Weather: Stecker und GPIO-Zuordnung](../mainboards/weatherstation/index.md)
- [Kompatibilitätsübersicht](../compatibility.md)

Die Anschlussform definiert weder ein pauschales Strombudget noch eine
automatische Firmwareunterstützung. Vor dem Verbinden Versorgung ausschalten,
Pinfolge und Ressourcen prüfen und danach die passende Funktion konfigurieren.
