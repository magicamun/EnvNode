# SCT013Head Rev. 0.1 – Erstprüfung

Stand: 2026-09-29. Erster Entwurfsstand, noch keine Fertigungsfreigabe.

## Aktueller Stand: Eingangsteiler R1/R4, 2026-09-30

Dieser Abschnitt ersetzt widersprechende ältere Prüfstände weiter unten.
R1 = R4 = 10 kOhm halbieren den Wechselanteil relativ zu VBIAS.
C1 = 10 nF liegt parallel zu R4; R2/R3 = 10 kOhm und C2 = 10 uF
bilden weiterhin die Vorspannung. Umsetzung in Schaltplan und Layout geprüft.
Tip/J1.3 führt über R1 zum ADC, Sleeve/J1.1 an VBIAS.

ERC: 0 Meldungen. Export meldet separat eine Annotationswarnung.
DRC nach Neuberechnung der Kupferflächen: 0 offene Verbindungen,
keine Kupferabstandsfehler. Die frühere VBIAS-Unterbrechung und das offene
Leiterbahnende sind behoben. PinHeader-Footprint und DNP sind angeglichen.
Noch offen: Referenz ADC im Schema versus ADC1 im Layout (2 Abgleichmeldungen),
1 aktive Drucküberlappung bei 3.3V und 9 ausgeschlossene Druckwarnungen.

Die Auslegung mit 10k/10k wurde vom Nutzer ausdrücklich gewählt.
Für 5 A / 1 V bei sinusförmigem Nennstrom: ADC ca. 0,943–2,357 V.
Dies ist keine garantierte Überstromfestigkeit oder Schutzbeschaltung.

Teileliste zum Commit unverändert vom Nutzer übernommen:
ENK-00010, Position 60: R2,R3,R4, ENP-00018, Stückzahl 3 bestätigt.
**BOM-Abweichung:** Position 50 führt R1 noch als ENP-00036 / 1 kOhm;
Schaltplan und Layout verlangen jetzt 10 kOhm. Vor Bestückung korrigieren.

## Abschlussprüfung 2026-09-30

Der zwischenzeitlich gespeicherte PCB-/Projektstand wurde erneut geprüft und
als Entwurf gesichert. ERC: 0 Fehler und Warnungen. DRC mit neu berechneten
Kupferflächen und Schaltplanabgleich bestätigt weiterhin die nachstehenden
Befunde: 1 offene VBIAS-Verbindung an J1.1, 5 aktive Warnungen (kurzes offenes
Leiterbahnende und vier 0,5-mm-Texte), 3 ausgeschlossene Randdruck-Warnungen
sowie 2 Abweichungen bei ADC1 (PinHeader im Schema / PinSocket im Layout und DNP).
Die gespeicherten Kupferflächen sehen teilweise verbunden aus; maßgeblich ist
die Prüfung nach Neuberechnung. Dieser Stand ist nicht fertigungsbereit.
Ober- und Unterseite wurden erneut visuell geprüft. Keine Änderungen am
Routing durch die Prüfung. Die kompakte Platinenkontur bleibt erhalten.

## Nachprüfung nach Pin-Korrektur und Beschriftung

Stand: 2026-09-29, einschließlich der während der Prüfung gespeicherten
Footprint-Änderung im Schaltplan; Layout 06:47:21.
Dieser Abschnitt ersetzt die untenstehenden Erstprüfungsbefunde, soweit geändert.

- J1.3 (Tip) ist jetzt korrekt mit R1 verbunden, J1.1 (Sleeve) mit VBIAS;
  Ring und Schaltkontakte sind unbeschaltet. Punkt 1 der Erstprüfung ist erledigt.
- ERC: weiterhin 0 Fehler und 0 Warnungen.
- DRC mit neu berechneten Kupferflächen und Schaltplanabgleich:
  **1 offene Verbindung**, 5 aktive Warnungen, 3 bereits ausgeschlossene
  Bestückungsdruck-Warnungen sowie 2 Schaltplan/Layout-Abweichungen.
- **Offene Verbindung:** J1.1/VBIAS hängt auf einer isolierten Kupferinsel;
  die Verbindung zur übrigen VBIAS-Fläche fehlt nach dem Umrouten.
  Vor Fertigung eine durchgehende Verbindung herstellen und neu prüfen.
- Kurzes offenes Leiterbahnende auf B.Cu bei (91,948; 57,557) mm,
  Netz Net-(J1-Pin_3), Länge 0,153 mm: bereinigen.
- Neue Pinbeschriftungen GND / +5V / ADC / 3.3V entsprechen der Portbelegung.
  +5V kennzeichnet nur die Position im Pigtail, der Head nutzt Pin 2 nicht.
  Alle vier Texte sind 0,5 mm hoch und unterschreiten die eingestellte
  Mindesthöhe von 0,8 mm; Drucklesbarkeit vor Fertigung verbessern.
- Punkt 2 der Erstprüfung ist im gespeicherten Stand noch offen: ADC1 besitzt
  im Schaltplan inzwischen Connector_PinHeader_2.54mm:PinHeader_1x04_P2.54mm_Vertical,
  auf dem Board weiterhin Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical.
  Auf dem Board ist DNP gesetzt, im Symbol nicht. Der erneute DRC-Abgleich
  nach dieser Speicherung bestätigt weiterhin die beiden Abweichungen.
- Top-/Bottom-Kupfer und Beschriftung erneut als SVG exportiert und visuell geprüft.
- Schaltung und Routing bei dieser Nachprüfung unverändert belassen.

## Konzept und Umfang

Kleine, von der verschraubten Lumberg-1503-09-Buchse getragene Platine,
ca. 20,9 × 14,0 mm. Passive Signalaufbereitung auf der Unterseite;
Pigtail zum ADC-Port. Ausschließlich SCT013 mit integrierter Bürde und
Spannungsausgang. Ausgangspunkt: 5 A / 0,333 V RMS.
Die geringe Platinengröße unterstützt die beabsichtigte geringe mechanische Last.
Kabelzug wird dadurch nicht begrenzt; Pigtail im Gehäuse separat zugentlasten.
Passprobe mit Buchse, Gehäusewand und Platine bleibt erforderlich.

## Elektrische Prüfung

R2/R3 = 10 kOhm bilden korrekt VBIAS = ca. 1,65 V aus 3,3 V.
C2 = 10 uF liegt korrekt zwischen VBIAS und GND.
R1 = 1 kOhm und C1 = 10 nF filtern das ADC-Signal gegen VBIAS.
Bei 5 A sinusförmig: ca. 0,471 V Spitze, ADC nominell 1,18–2,12 V.
ADC-Konfiguration und Kalibrierung müssen diesen Bereich abdecken.

ADC1: Pin 1 GND, Pin 2 unbenutzt, Pin 3 ADC, Pin 4 +3V3_SYS.
Dies entspricht den neuen Mini-ADC-Ports; die dort angebotenen 5 V auf
Pin 2 werden im Head nicht genutzt.

## Offene Punkte vor Fertigung

1. **Sensor-Signalkontakt prüfen/korrigieren.** J1.1 (Sleeve) liegt an VBIAS,
   J1.2 (Ring) an R1. J1.3 (Tip) ist unbeschaltet. Die dokumentierte
   YHDC-Standardbelegung nutzt Tip/Sleeve, Ring bleibt frei. Für diesen
   Standard muss das Messsignal an J1.3 statt J1.2 geführt werden, in
   Schaltplan UND Layout. Vor Änderung die konkrete 5-A-Ausführung prüfen.
   Die ERC/DRC-Prüfung erkennt eine solche semantische Pinverwechslung nicht.
2. **ADC1-Footprint synchronisieren.** Schaltplan:
   Connector_Wire:SolderWire-0.25sqmm_1x04_P4.2mm_D0.65mm_OD1.7mm;
   Layout: Connector_PinSocket_2.54mm:PinSocket_1x04_P2.54mm_Vertical.
   Auch DNP unterscheidet sich. Für die kompakte Platine ist die vorhandene
   2,54-mm-Padreihe mechanisch plausibel, wenn die konkrete Litze in die
   1-mm-Bohrungen passt. Eine konsistente Lötaugen-Variante ohne Buchsenmodell
   wäre sinnvoll; ein automatisches Update auf 4,2 mm würde das Layout verändern.
3. **Bestückungsdruck am Rand.** Drei bereits ausgeschlossene Meldungen
   betreffen den J1-Umriss an der linken Platinenkante. Kein Kupferfehler;
   die betroffenen Linien werden bei der Fertigung möglicherweise abgeschnitten.
4. Sensor-Kompatibilität (integrierte Bürde, 0,333-V-Ausgangspunkt), Pinbelegung
   des Pigtails und Kabelfarben in der Bestückungsdokumentation festhalten.

## Durchgeführte Prüfungen

- KiCad 10.0.5 ERC: 0 Fehler, 0 Warnungen.
- DRC des gespeicherten Layouts: 0 aktive Verletzungen, 0 offene Verbindungen.
- DRC mit neu berechneten Kupferflächen (nicht gespeichert),
  Schaltplanabgleich und eingeschlossenen Ausnahmen: 3 ausgeschlossene
  Bestückungsdruck-Warnungen; 2 Warnungen zu Footprint/Bestückungsattributen
  von ADC1; 0 offene Pads.
- Kupferlagen und Bestückungsdruck als SVG exportiert und visuell geprüft.
- Keine Änderungen an Schaltung oder Routing im Rahmen dieser Prüfung.

## Referenzen

- Lumberg-Pinbelegung und Mechanik:
  https://cdn-reichelt.de/documents/datenblatt/C160/DS_1503_09.pdf
- YHDC-Datenblatt der 0,333-V-Familie (Herstellerdokument, Spiegel):
  https://innovatorsguru.com/wp-content/uploads/2019/07/SCT-013-Datasheet.pdf
- Dokumentierte YHDC-Tip/Sleeve-Belegung (Messbericht):
  https://docs.openenergymonitor.org/electricity-monitoring/ct-sensors/yhdc-sct-013-000-ct-sensor-report.html
