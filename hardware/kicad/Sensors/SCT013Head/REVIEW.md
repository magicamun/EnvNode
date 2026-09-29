# SCT013Head Rev. 0.1 – Erstprüfung

Stand: 2026-09-29. Erster Entwurfsstand, noch keine Fertigungsfreigabe.

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
