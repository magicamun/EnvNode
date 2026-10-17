<!-- EnvNode-Lizenz: siehe LICENSE im Repository-Stamm. -->
# Externe Anschlusssets

Die Anschlusssets bestehen aus losen Komponenten. Die Belegung folgt stets den
Pinnummern: **Pin 1 an Pin 1, Pin 2 an Pin 2 usw.**

## Varianten und Lieferumfang

| Variante | WEIPU am Kabel | WEIPU am Gehäuse | Internes Pigtail | Außenleitung |
| --- | --- | --- | --- | --- |
| JST-PH, 4-polig | SP1310/P4I-N | SP1312/S4-N | PH-kompatibel, 100 mm | 500 mm, 4 × 0,5 mm² |
| JST-SH, 4-polig | SP1310/S4I-N | SP1312/P4-N | SH-kompatibel, 150 mm | 500 mm, 4 × 0,5 mm² |
| 5-V-Versorgung, 2-polig | SP1310/P2I-N | SP1312/S2-N | PH-kompatibel, 100 mm | Nicht enthalten |

Jedes Set enthält eine Schutzkappe **CAP-SP1311/12** ausschließlich für den
Gehäuseanschluss. Die vieradrige Außenleitung ist LAPP ÖLFLEX ROBUST 210 4X0,5,
Herstellerartikel 0021884, mit nominal 5,8 mm Außendurchmesser. Die WEIPU-I-Ausführung
ist für 4–6,5 mm Kabelaußendurchmesser vorgesehen.

PH und SH verwenden bewusst die umgekehrte Zuordnung von Stift- und Buchsenkontakten.
Ein Gehäuse oder Netzteil gehört nicht zum Anschlussset.

## Verdrahtung

| JST-Pin | WEIPU am Gehäuse | WEIPU am Kabel |
| --- | --- | --- |
| 1 | 1 | 1 |
| 2 | 2 | 2 |
| 3, nur vierpolig | 3 | 3 |
| 4, nur vierpolig | 4 | 4 |

Bei der nummerierten vieradrigen Leitung wird Ader 1 für Pin 1 verwendet, Ader 2
für Pin 2 usw. Maßgeblich sind die eingeprägten WEIPU-Pinnummern und die
Pin-1-Kennzeichnung des JST-Anschlusses. Steck- und Lötseitenansichten nicht
ungeprüft übertragen; Aderfarben sind kein Beleg für die Pinbelegung.

Das Anschlussset überträgt die Belegung des jeweiligen Boardanschlusses unverändert.
Vor dem Verbinden prüfen, dass beide Gegenstellen dieselbe Signalbelegung erwarten.
Insbesondere werden Versorgungsspannung und Masse nicht durch die Bezeichnung
„1:1“ neu festgelegt.

## Kontrolle vor Inbetriebnahme

Spannungsfrei jeden Pfad auf Durchgang n→n prüfen und unbeabsichtigte Verbindungen
zwischen unterschiedlichen Pins ausschließen. Den ersten Aufbau einschließlich
Zugentlastung und Abdichtung prüfen; die Schutzart eines fertigen Aufbaus ergibt
sich nicht allein aus der Auswahl eines IP68-Steckverbinders.

Für PH gilt als Auslegungsziel höchstens 2 A, für SH höchstens 1 A, jeweils nur bei
geeigneten Kontakten und Leitungen. Für kompatible Pigtails muss deren konkrete
Belastbarkeit berücksichtigt werden.
