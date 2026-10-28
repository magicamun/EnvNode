---
title: RainDetectorHead 0.2 - Bestückung und Inbetriebnahme
pdf:
  id: rain-head-0.2
  title: RainDetectorHead
  subtitle: Bestückungs- und Bring-up-Anleitung
  hardware_revision: '0.2'
  document_version: '1.1'
  document_date: '2026-10-28'
  source_document_date: '2026-10-28'
  status: Dokumentation abgeschlossen - Hardwareprüfstand siehe Anleitung
---

# RainDetectorHead 0.2 - Bestückung und Inbetriebnahme

Passiver kapazitiver Sensorkopf mit fünf parallelen Heizsträngen und insgesamt zwanzig 15-Ω-Widerständen.

**Dokumentation abgeschlossen am 28. Oktober 2026.** Der Fertigungs- und Erprobungsstand wird im Abschlusskapitel getrennt geführt.

[Anleitung als PDF](../../assets/downloads/RainDetectorHead_Rev0.2_Anleitung.pdf){ .md-button }
[Alte Revisionen](old-revisions.md){ .md-button }
### Änderungen und Geltungsbereich

Aktuelle Paarung: **RainDetectorController 0.4 (ENP-00138)** mit **RainDetectorHead 0.2 (ENP-00139)**. Beide Konturen messen **62,12 × 54,52 mm** an der Mittellinie von Edge.Cuts. Die neue Form berücksichtigt die Gehäusedichtung und versetzte Montagebohrungen. Keine neue Firmware-Mindestversion folgt allein aus dieser mechanischen Revision.

## 3D-Ansicht

![rain-head-0.2: 3D-Vorderseite](../../assets/images/rain-head-0.2/top.png){ .hero-paired-view }
![rain-head-0.2: 3D-Rückseite](../../assets/images/rain-head-0.2/bottom.png){ .hero-paired-view }

Vorderseite (F.Cu) und bestückte Rückseite (B.Cu). Nicht hinterlegte 3D-Modelle bleiben ausgespart. Die Bestückungspläne folgen im Abschnitt zur Bestückungsübersicht.

## 1. Teile und Vorbereitung

### 1.1 Elektronische Bestückung

| Ref. | Anz. | Wert / Funktion | EnvNode-Part | Gehäuse / Footprint |
| --- | --- | --- | --- | --- |
| R1-R20 | 20 | 15 Ohm, 1 %, 250 mW; WR12X15R0FTL | ENP-00115 | 1206; B.Cu |
| J1, J2 | 1 | 2x10-SMD-Leiste für zwei 2x2-Zuschnitte | ENP-00111 | 2,54 mm; B.Cu |
| PCB | 1 | RainDetectorHead PCB Rev. 0.2 | ENP-00139 | 62,12 × 54,52 mm |

Die Tabelle nennt den Bedarf pro Platine. Die bisherige Kitnummer bestätigt keine auf den neuen PCB-Artikel umgestellte Kit-BOM.

Zuschnitt: Menge 1 bei ENP-00111 bezeichnet eine ganze SL 2X10G SMD2,54. Daraus entstehen zwei 2x2-Stecker für J1/J2. Je Zuschnitt drei Doppelkontaktpositionen einschließlich Schnittverlust vorsehen; Reststücke werden nicht gutgeschrieben.

C1 ist die gefertigte Kupfer-Sensorstruktur und kein zusätzliches Kauf- oder Lötbauteil. H1-H4 sind vier 3,2-mm-NPTH-Bohrungen; M3-Schrauben, Abstandshalter und Dichtung sind nicht Bestandteil dieses Bausatzes.

### 1.2 Sensorfläche und Fertigung

C1 nutzt RainDetector_Interdigitated_44.78x37.25mm_P0.51mm. Die Sensorpads haben keine F.Mask- oder F.Paste-Öffnungen. Der Mindestabstand 0,10 mm gilt nur zwischen Objekten von C1; sonst gelten die normalen Regeln.

Die beiden Vias sind vorderseitig mit Plugging und Tenting versehen; Filling und Capping sind deaktiviert (Type-IV-a-Absicht). Verschlussprozess und Wasserausschluss mit dem Fertiger vereinbaren. Tenting allein garantiert keine Abdichtung.

Sensorfläche nicht freilegen, verzinnen oder mit Lötpaste versehen. Beschichtung und Reinigung beeinflussen Isolation und Kapazität. Steckhöhe und mechanische Paarung mit den Controller-Buchsen am Muster prüfen.

### 1.3 Artikel- und Dokumentstand

Bauteilwerte und Anschlusszuordnung wurden aus dem aktuellen KiCad-Stand übernommen. Standardartikel stammen aus der bisherigen Anleitung; die neuen PCB-Artikel ENP-00138/ENP-00139 sind im mechanischen Projektstand vom 21.10.2026 dokumentiert. Keine neue Lager- oder Kit-BOM-Prüfung wird behauptet.

EnvNode-eigene Inhalte: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe LICENSE im Repository. Drittanbieterrechte bleiben unberührt.

## 2. Bestückungsübersicht

### Vorderseite: Bestückungsplan

![rain-head-0.2: senkrechter Bestückungsplan Vorderseite](../../assets/board-views/rain-head-0.2/assembly-front.png){ .assembly-plan }

[Plan als PNG](../../assets/board-views/rain-head-0.2/assembly-front.png){ .md-button }

### Rückseite: Bestückungsplan

![rain-head-0.2: senkrechter Bestückungsplan Rückseite](../../assets/board-views/rain-head-0.2/assembly-back.png){ .assembly-plan }

[Plan als PNG](../../assets/board-views/rain-head-0.2/assembly-back.png){ .md-button }

Senkrechtes KiCad-Rendering der Leiterplatte ohne Bauteilmodelle. Kupfer, Lötstoppmaske und Bestückungsdruck entsprechen der PCB-Quelle. Rückseitenpläne werden von B.Cu aus betrachtet.


Die Bestückungspläne zeigen die Leiterplatte senkrecht zur jeweiligen Seite. Bestückungsdruck und Referenzen folgen der PCB-Quelle; eine Rückseitenansicht ist keine Kontaktansicht eines Kabelsteckers.

B.Cu: Hier liegen alle Heizwiderstände sowie J1/J2. In der vorgesehenen Montage zeigen Head und Controller mit B.Cu zueinander. Auf der Controller-Seite sitzen die Buchsen; die aktiven Controller-Bauteile bleiben von innen zugänglich.

## Löt-Reihenfolge

1. Widerstände mit Messung oder belastbarer Kennzeichnung als 15 Ohm identifizieren.
2. R1 bis R20 bestücken. Jeweils einen Heizstrang vollständig prüfen; keine Position auslassen.
3. J1/J2 ausrichten und zunächst je ein Pad fixieren. Kontaktachsen und Rechtwinkligkeit prüfen, danach alle Pads löten.
4. Flussmittelreste materialverträglich entfernen; Sensorfläche und Maskierung nicht beschädigen.
5. Lötstellen, Via-Verschluss, Platinenkanten und Montageabstände prüfen. Vor dem Zusammenstecken die Prüfungen in Abschnitt 4 durchführen.

Der Abstand zwischen den Platinen muss für Widerstände, Steckverbinder, Lötstellen und Befestigung ausreichen. Schrauben und Scheiben dürfen keine Leiterbahnen berühren oder die Sensorfläche beschädigen. Pinfolge nicht allein aus der gespiegelten Unterseitenansicht ableiten.

## 3. Kontakte und Heiznetzwerk

| Head | Funktion | Controller Rev. 0.4 |
| --- | --- | --- |
| J1.1 | HEATER+ / +5V | J1.1 |
| J1.4 | GND / Elektrode 2 | J1.4 |
| J2.1 | SENSE / Elektrode 1 | J2.1 |
| J2.4 | HEATER- / geschalteter Rückleiter | J2.4 |
| J1/J2 Pins 2 und 3 | Elektrisch unbeschaltet | Elektrisch unbeschaltet |

Die Tabelle bildet die Netzzuordnung der aktuellen Quellen ab. SENSE liegt auf J2.1, HEATER- auf J2.4. Orientierung und Kontaktpaarung vor Montage durch eine mechanische Passprobe und Durchgangsprüfung bestätigen.

### Fünf parallele Heizstränge

| Strang | Vier Widerstände in Reihe | Nennwiderstand |
| --- | --- | --- |
| 1 | R1, R2, R3, R4 | 60 Ohm |
| 2 | R5, R6, R7, R8 | 60 Ohm |
| 3 | R9, R10, R11, R12 | 60 Ohm |
| 4 | R13, R14, R15, R16 | 60 Ohm |
| 5 | R17, R18, R19, R20 | 60 Ohm |

Gesamt: 60 Ohm / 5 = 12 Ohm . Bei 5 V: I = 5 / 12 = 0,417 A ; P = 25 / 12 = 2,08 W . Je Widerstand entfallen nominal etwa 0,104 W . Das sind Rechenwerte ohne Bauteiltoleranzen, Leitungsverluste oder Temperaturänderung.

HEATER- ist der vom Controller geschaltete Rückleiter und keine dauerhaft verbundene Masse. Der Controller verbindet ihn über Q1 mit GND. Ein externes Kurzschließen von HEATER- nach GND würde die Abschaltung über Q1 umgehen.

## 4. Spannungsfreie Prüfung

- ☐ Alle zwanzig Widerstände bestückt; keine Brücken oder fehlenden Lötstellen.
- ☐ J1/J2 korrekt ausgerichtet und mechanisch belastbar gelötet.
- ☐ Zwischen J1.1 und J2.4 etwa 12 Ohm, abhängig von Widerstandstoleranz und Messleitungen.
- ☐ J2.1 hat Durchgang zu C1.1 (SENSE).
- ☐ J1.4 hat Durchgang zu C1.2 (GND-Elektrode).
- ☐ Kein niederohmiger Kurzschluss zwischen SENSE und GND.
- ☐ Heizkreis ohne niederohmige Verbindung zu SENSE oder GND-Elektrode, solange der Controller getrennt ist.
- ☐ Pins 2 und 3 beider Stecker bleiben unbeschaltet.
- ☐ Maske über der Sensorfläche intakt; keine Kratzer, Kupferfreilegung oder Verschmutzung.
- ☐ Vorderseitiger Via-Verschluss gemäß vereinbartem Fertigungsprozess geprüft.

### Widerstandsbefunde einordnen

Ein vollständig offener Heizstrang erhöht den Gesamtwiderstand rechnerisch von 12 auf 15 Ohm. Ein zu kleiner Wert kann auf falsche Widerstände oder Brücken hinweisen. Die fünf Stränge anhand der Referenzen prüfen; eine In-Circuit-Einzelmessung kann durch die übrigen Zweige beeinflusst werden.

### Mechanik und Beschichtung

- ☐ Head und Controller ohne Versatz paarbar; keine Pin-gegen-Pin-Fehlzuordnung.
- ☐ Steckhöhe und M3-Befestigung verhindern Kontakt zwischen den Platinen.
- ☐ Montage hält den Controller trocken und entlastet die Steckverbinder.
- ☐ Beschichtungswerkstoff und Aushärtung dokumentiert; Sensorfläche reproduzierbar.
- ☐ Verschluss der Vias und Kanten im vorgesehenen Einsatz separat auf Wassereintritt geprüft.

Eine Multimeterprüfung auf Kurzschluss ist keine nachgewiesene Feuchte- oder Isolationsqualifikation. Eine trockene Grundkapazität beziehungsweise Grundfrequenz wird erst am fertigen Aufbau als Referenz erfasst.

## 5. Erstprüfung und Messprotokoll

### Heizung separat prüfen

1. Controller abziehen. Ein strombegrenztes 5-V-Labornetzteil an J1.1 (+) und J2.4 (-) anschließen; zunächst etwa 0,5 A begrenzen.
2. Nur beaufsichtigt einschalten. Spannung direkt am Head und Gesamtstrom messen; nominal etwa 417 mA bei 5 V.
3. Erwärmung der Widerstände, Leiterplatte und Stecker beobachten. Bei ungleichmäßiger oder auffälliger Erwärmung abschalten und Ursache prüfen.
4. Ausschalten und abkühlen lassen. Widerstand und Sichtzustand erneut kontrollieren. Zulässige Temperatur und Einschaltdauer müssen vor Dauerbetrieb festgelegt werden.

### Sensor zusammen mit dem Controller prüfen

Spannungsfrei gemäß Abschnitt 3 verbinden. Zuerst den Controller mit 3,3 V bei ausgeschalteter Heizung prüfen. RAIN_FREQ am Controller beobachten und trockene Grundfrequenz erfassen. Kontrollierte Benetzung nur auf der dafür vorgesehenen Head-Fläche; Controller und Anschlüsse trocken halten. Frequenzänderung und Rückkehr beim Trocknen dokumentieren. Anschließend Heiz-Ein/Aus-Verhalten nach der Controller-Anleitung prüfen.

| Messung | Erwartung / Dokumentation | Ist / Datum |
| --- | --- | --- |
| Heizwiderstand kalt | Nominal 12 Ohm |  |
| Heizspannung und -strom | 5 V; rechnerisch ca. 417 mA |  |
| Temperaturverlauf | Head, Widerstände, Stecker; Dauer erfassen |  |
| Trockene Grundfrequenz | Montage, Temperatur und Beschichtung notieren |  |
| Benetzung und Trocknung | Frequenzverlauf und Wiederholbarkeit |  |
| Heizung über Controller AUS | Heizstrom geht zurück |  |
| Nach dem Test | Maske, Steckkontakte, Widerstand unauffällig |  |

Keine festen Regen-Schwellwerte aus dieser Anleitung ableiten. Die Firmware soll Rohmesswerte bereitstellen; Interpretation und Automatisierung bleiben in höheren Softwareebenen. Die PCNT-Lab-Firmware steuert die Heizung nicht.

## 6. Abschluss und Revisionsnachweis

Dokumentation abgeschlossen für Head 0.2 / Controller 0.4. Die vergrößerte Kontur schafft Platz für die Gehäusedichtung; Bohrungen wurden mit der Kontur verschoben. Die Kontaktachsen von J1/J2 und die elektrische Netzzuordnung bleiben gegenüber dem unmittelbar vorherigen gespeicherten PCB unverändert. Beim Head ist die aktive Struktur im aktuellen Footprint 44,78 × 37,25 mm mit 0,51-mm-Raster dokumentiert; der frühere Guide mit 48,86 mm benennt den aktuellen Footprint nicht korrekt.

Quellvergleich und neue Renderings belegen keine gefertigte Dichtung, mechanische Passprobe, Beschichtung, Via-Abdichtung oder reale Heiz-/Regenprüfung. Es wird hier kein neuer bestandener vollständiger ERC-/DRC-Lauf behauptet. Die alten Bring-ups stehen unter [Alte Revisionen](old-revisions.md).

### Quellenstand

Dokumentiert ist der gespeicherte KiCad-Stand aus `rain-detector`, Projekt `RainDetectorHead`, Basiscommit `e3ecde6c66d24c26fbb0352b686f2d3f10469c72`. Die Prüfsummen bestimmen den tatsächlichen Arbeitsstand einschließlich lokaler Änderungen; der Commit allein beschreibt diese nicht.

| Quelle | SHA-256 |
| --- | --- |
| RainDetectorHead.kicad_dru | `1203b82235c87187154b35954664d914d52d4b1e6ea8024e5ddf9a524a09af13` |
| RainDetectorHead.kicad_pcb | `6cf50cba758762ee190e0aff3aae11c6274afda0fded1f36e0ee592886aab438` |
| RainDetectorHead.kicad_pro | `af1e22e6e6ffbad3b409b996fbc377dfea7da8e643027662e977fe79b5bd77ce` |
| RainDetectorHead.kicad_sch | `82bface8c81a513f7d1c88ebcdf12783961440fefcb477361c4d102ed4f36cfe` |

[Rendernachweis](../../assets/images/rain-head-0.2/provenance.json). Dokumentation: LicenseRef-EnvNode-Use-and-Redistribution-1.0, siehe [Lizenz](../../license.md). Drittanbieterrechte an KiCad-Modellen und Bibliotheken bleiben bestehen.
