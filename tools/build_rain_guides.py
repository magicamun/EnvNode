#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../LICENSE. Native KiCad renders retain the model providers' rights.
"""Build the current mechanical RainDetector guides from reviewed source data."""
from pathlib import Path
import hashlib, json
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image, PageBreak
from reportlab.lib.styles import getSampleStyleSheet, ParagraphStyle
from reportlab.lib.colors import HexColor, white
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
ROOT=Path(__file__).resolve().parents[1]
FONTS=Path('/System/Library/Fonts/Supplemental')
for n,f in [('Guide','Arial.ttf'),('GuideBold','Arial Bold.ttf')]:pdfmetrics.registerFont(TTFont(n,str(FONTS/f)))
styles=getSampleStyleSheet()
for n in ['Normal','BodyText','Heading1','Heading2','Title']:
 styles[n].fontName='GuideBold' if n in ['Title','Heading1','Heading2'] else 'Guide'
 styles[n].textColor=HexColor('#243746')
styles['BodyText'].fontSize=10;styles['BodyText'].leading=14
styles.add(ParagraphStyle('small',fontName='Guide',fontSize=8,leading=11,textColor=HexColor('#526779')))
def P(s,style='BodyText'):return Paragraph(s,styles[style])
def table(rows,widths):
 t=Table([[P(str(v),'small') for v in row] for row in rows],colWidths=widths,hAlign='LEFT')
 t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),HexColor('#e8eef2')),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7),('LINEBELOW',(0,0),(-1,-1),.3,HexColor('#d5dde3'))]))
 return t
def section(title,text):return [P(title,'Heading2'),P(text),Spacer(1,8)]
def build(name,rev,part,partner):
 folder=ROOT/f'hardware/kicad/Sensors/RainDetector{name}'
 out=folder/f'Documentation/RainDetector{name}_Bestueckungs_und_Bringup_Rev{rev}.pdf'
 out.parent.mkdir(exist_ok=True)
 story=[P(f'RAINDETECTOR {name.upper()}','Title'),P(f'Bestückung und Bring-up | PCB Revision {rev}','Heading1'),P(f'21. Oktober 2026 | {part} | Partner: {partner}'),P('Mechanikstand: 62,12 x 54,52 mm. Größere Kontur für die Gehäuseabdichtung; C1 und J1/J2 bleiben gegenüber dem letzten PCB-Stand unverändert.'),Spacer(1,12)]
 for side,label in [('top','F.Cu: Sensorseite' if name=='Head' else 'F.Cu: Bestückungsseite'),('bottom','B.Cu: Heizwiderstände und Interconnects' if name=='Head' else 'B.Cu: Buchsen zum Head')]:
  story += [P(label,'Heading2'),Image(str(ROOT/f'images/rain-detector/RainDetector{name}-Rev{rev}-{side}.png'),width=455,height=310),Spacer(1,4)]
 # Both images get their own overview page to preserve detail and avoid clipping.
 story.insert(8,PageBreak())
 story += [PageBreak(),P('1. Teile und Mechanik','Heading1')]
 if name=='Head':
  rows=[['Ref.','Anzahl','Wert / Artikel','Bauform'],['R1-R20','20','15 Ohm, 1 %, 250 mW; ENP-00115','1206; B.Cu'],['J1/J2','1 Strip','ENP-00111: 2x10-SMD-Leiste; zwei 2x2-Zuschnitte','2,54 mm; B.Cu'],['PCB','1',part+' / Rev. '+rev,'62,12 x 54,52 mm']]
 else:
  rows=[['Ref.','Anzahl','Wert / Artikel','Bauform'],['C1','1','100 nF, X7R; ENP-00006','1206'],['R1','1','100 Ohm; ENP-00017','1206'],['R2/R4','2','100 kOhm; ENP-00016','1206'],['R3','1','10 kOhm; ENP-00018','1206'],['Q1','1','AO3400A; ENP-00013','SOT-23'],['U1','1','ICM7555IBAZ; ENP-00020','SOIC-8'],['J1/J2','2','BKL 10120957; ENP-00116','2x2, 2,54 mm; B.Cu'],['PCB','1',part+' / Rev. '+rev,'62,12 x 54,52 mm']]
 story += [table(rows,[55,55,280,117]),Spacer(1,10),P('Die Artikelnummern der unveränderten Bauteile stammen aus dem bisherigen Bestückungsstand. <b>Kits und Assemblies sind noch nicht an die neue PCB-Revision angepasst</b>; diese Anleitung bestätigt keine aktualisierte Kit-Stückliste. Schrauben, Abstandshalter und Dichtung separat festlegen.')]
 if name=='Head':story += [P('C1 ist gefertigte Kupferstruktur, kein Kaufbauteil. Eine ganze ENP-00111-Leiste liefert zwei 2x2-Zuschnitte; Schnittverlust berücksichtigen.')]
 else:story += [P('J3/J4 sind im Schaltplan DNP. Anschlussvariante und zugentlastete Kontaktierung separat festlegen. Ein Rendering kann auch DNP-Positionen zeigen; der Bestückungsplan ist maßgeblich.')]
 story += section('Montage','Vier 3,2-mm-NPTH-Bohrungen für M3. Beide B.Cu-Seiten zeigen zueinander. Kontaktachsen, reale Steckhöhe, Schraubenfreiheit und Dichtungsauflage am Muster prüfen. Die neue Kontur allein ist kein Nachweis der Dichtheit.')
 if name=='Head':coords=[['H1/H2','3,676955 / 58,443045','3,676955'],['H4/H3','3,676955 / 58,443045','50,843045']]
 else:coords=[['H1/H2','3,680 / 58,440','3,680'],['H4/H3','3,680 / 58,440','50,940']]
 story += [P('Bohrungsmittelpunkte in mm ab linker/oberer Konturkante (F.Cu-Koordinaten):'),table([['Bohrungen','X links / rechts','Y']]+coords,[110,240,157]),P('Die tatsächlichen Koordinaten sind je PCB angegeben; sie sind keine gemeinsame Bohrschablone. Insbesondere unterscheiden sich die unteren Y-Positionen geringfüggig. Vor mechanischer Fertigung gegen die Gehäusekonstruktion prüfen.','small')]
 story += [PageBreak(),P('2. Elektrische Zuordnung','Heading1'),table([['Head','Signal','Controller'],['J1.1','HEATER+ / +5V','J1.1'],['J1.4','GND / Elektrode 2','J1.4'],['J2.1','SENSE / Elektrode 1','J2.1'],['J2.4','HEATER- / geschalteter Rückleiter','J2.4'],['J1/J2 Pins 2 und 3','Elektrisch unverbunden','Elektrisch unverbunden']],[100,307,100])]
 story += section('Controller zum Mainboard','J3: 1 GND, 2 +5V, 3 HEATER_EN, 4 +3V3.<br/>J4: 1 GND, 2 +5V, 3 RAIN_FREQ, 4 +3V3.<br/>Versorgungspins sind gemeinsam geführt. Keine unabhängigen Netzteile parallel verbinden. HEATER_EN und RAIN_FREQ sind 3,3-V-Signale; keine 5 V an ESP32-Signale oder +3V3 anschließen.')
 story += section('Heizung','Fünf parallele Stränge mit je vier 15-Ohm-Widerständen: R1-R4, R5-R8, R9-R12, R13-R16, R17-R20. Je Strang 60 Ohm, gesamt 12 Ohm. Bei 5 V nominal 417 mA und 2,08 W; etwa 0,104 W pro Widerstand. Widerstände mit mindestens 0,25 W einsetzen und Temperatur-Derating beachten. Diese Werte sind berechnet, keine Messung.')
 story += [P('HEATER- ist kein dauerhaftes GND: Q1 verbindet den Rückleiter mit Masse. Externes Kurzschließen nach GND umgeht die Abschaltung.')]
 story += section('Sensorfläche und Fertigung','C1-Footprint: RainDetector_Interdigitated_44.78x37.25mm_P0.51mm. Keine F.Mask-/F.Paste-Öffnungen über den Sensorpads; Fläche nicht verzinnen, freilegen oder mit Lötpaste versehen. Der 0,10-mm-Abstand ist nur für C1 vorgesehen. Via-Plugging und vorderseitiges Tenting mit dem Fertiger abstimmen; Filling/Capping sind nicht als Type VII freigegeben. Maskententing allein garantiert keine Abdichtung.')
 story += [PageBreak(),P('3. Montage und Erstprüfung','Heading1')]
 if name=='Head':
  story += section('Bestückung','R1-R20 als 15 Ohm identifizieren und vollständig auf B.Cu bestücken. J1/J2 zunächst einseitig fixieren, Kontaktachsen kontrollieren und fertig löten. Flussmittel materialverträglich entfernen. Maskierung, Vias und Kanten nicht beschädigen.')
  checks=['Heizpfad J1.1 nach J2.4 nominal 12 Ohm; ein offener Strang ergibt rechnerisch 15 Ohm.','J2.1 nach C1.1 (SENSE), J1.4 nach C1.2 (GND-Elektrode) prüfen.','Kein niederohmiger Kurzschluss zwischen SENSE und GND oder zwischen Heizpfad und Elektroden bei getrenntem Controller.','Pins 2/3 unverbunden; Maske intakt; Via-Verschluss, Steckhöhe und Isolierung prüfen.']
 else:
  story += section('Bestückung','C1 und R1-R4 bestücken; 100 Ohm nicht mit 100 kOhm verwechseln. U1/Q1 ausrichten und Pin 1 prüfen. J1/J2 auf B.Cu montieren. U1.1 = GND, U1.4/U1.8 = +3V3. Q1.1 = Gate, Q1.2 = Source/GND, Q1.3 = Drain/HEATER-.')
  checks=['Kein Kurzschluss zwischen +3V3, +5V und GND.','J3.1/J4.1/Q1.2 an GND; J3.2/J4.2 an J1.1 (+5V).','J3.4/J4.4/C1.1/U1.4/U1.8 an +3V3; C1.2 an GND.','J3.3 über R1 = 100 Ohm zum Gate; R2 = 100 kOhm von Gate nach GND.','Q1.3 nach J2.4; J2.1 nach U1.2/U1.6; J4.3 nach U1.3 prüfen.','Pins 2/3 von J1/J2 bleiben unverbunden.']
 story += [P('Spannungsfrei prüfen','Heading2')]+[P('[ ] '+x) for x in checks]
 story += section('Sensor zuerst, Heizung separat','Zuerst ohne 5-V-Heizversorgung und mit HEATER_EN LOW prüfen. Controller mit strombegrenzten 3,3 V versorgen (20 mA vorsichtiger Startwert, kein Sollstrom). Versorgung/Orientierung prüfen; dann spannungsfrei den Head aufstecken. RAIN_FREQ gegen GND mit Oszilloskop/Frequenzzähler prüfen. Ohne Head ist keine definierte Grundfrequenz zugesichert.')
 story += section('Heiztest','5 V mit zunächst etwa 0,5 A Strombegrenzung zuschalten und beaufsichtigen. LOW: Heizung aus. HIGH mit 3,3 V: nominal 417 mA zusätzlich zu Nebenlasten. Versorgungseinbruch und Temperaturen an Q1, Head und Kontakten prüfen. Danach LOW: Heizstrom muss zurückgehen. Steuersignal trennen und Ausschaltung prüfen. Bei Auffälligkeiten abschalten.')
 story += [PageBreak(),P('4. Firmware, Protokoll und Quellen','Heading1')]
 story += section('Aktueller Firmwarestand','Die gemeinsame Firmware enthält getrennte Sensor-, Aktor- und Controller-Implementierungen. Aktivierte RainDetector-Sensoren/Controller starten nach Boot; die Heizung startet aus. Neue Benetzung fordert 100 % an. Regenende folgt voreingestellt nach 60 s ohne neue erkannte Benetzung; Heizung bleibt bis bestätigter Trockenfrequenz an. 32/34 kHz sind einstellbar. Das 5000-ms-Messintervall steuert Veröffentlichungen, die lokale Erfassung arbeitet etwa alle 250 ms. Keine Temperatur-/Taupunktregelung oder SD-Feldlogs in diesem Stand. Einzelheiten: docs/RainDetector.md.')
 story += [table([['Protokollpunkt','Messwert / Datum'],['+3V3; +5V unter Last; Ruhestrom',''],['Frequenz trocken / benetzt / abtrocknend',''],['Heizstrom; Temperaturen; Testdauer',''],['Abschaltung bei Trocken / Signalverlust',''],['Montage, Beschichtung, Via-Verschluss, Wasserausschluss','']],[300,207])]
 story += [P('Quellenfingerprints (SHA-256)','Heading2')]
 for suffix in ['kicad_pcb','kicad_sch','kicad_pro']:
  source=folder/f'RainDetector{name}.{suffix}';story += [P(source.name,'small'),P(hashlib.sha256(source.read_bytes()).hexdigest(),'small')]
 story += [Spacer(1,8),P('Prüfumfang 21.10.2026: PCB-Kontur, Bohrungen, unveränderte C1-/J1-/J2-Lage und Pad-Netzzuordnungen gegen den zuvor committed PCB-Stand verglichen; neue native KiCad-Renderings. Kein neuer vollständiger ERC/DRC-Lauf, keine Fertigungsfreigabe und kein physischer Test der neuen mechanischen Revision. Die Laborergebnisse betreffen den bisherigen Aufbau.','small'),P('Renderings: KiCad 10.0.5, perspektivisch; oben -25/0/5 Grad, unten -25/0/-5 Grad. Nicht modellierte Gehäuse und reale Beschichtungen werden nicht nachgebildet. Drittanbieter-Modelle behalten ihre Lizenzen. EnvNode-eigene Inhalte: siehe LICENSE (LicenseRef-EnvNode-Use-and-Redistribution-1.0).','small')]
 def footer(c,doc):
  c.setFont('Guide',8);c.setFillColor(HexColor('#526779'));c.drawString(44,818,f'ENVNODE | RAINDETECTOR {name.upper()} REV. {rev} | {part}');c.drawString(44,25,'21. Oktober 2026 | Bestückung und Bring-up | Mechanikstand zur Prüfung');c.drawRightString(551,25,str(doc.page))
 SimpleDocTemplate(str(out),pagesize=(595.276,841.89),leftMargin=44,rightMargin=44,topMargin=45,bottomMargin=45,title=f'RainDetector{name} Rev. {rev}',author='EnvNode').build(story,onFirstPage=footer,onLaterPages=footer)
 print(out)
if __name__=='__main__':
 build('Head','0.2','ENP-00139','Controller 0.4')
 build('Controller','0.4','ENP-00138','Head 0.2')
