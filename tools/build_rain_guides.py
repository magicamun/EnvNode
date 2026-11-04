#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../LICENSE. Native KiCad renders retain the model providers' rights.
"""Build RainDetector guides using the DuoRelay guide's structure and visual style."""
from pathlib import Path
import hashlib
from reportlab.platypus import SimpleDocTemplate, Paragraph, Spacer, Table, TableStyle, Image, PageBreak
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.colors import HexColor, white
from reportlab import rl_config
rl_config.useA85 = 0  # Binary PDF streams; avoid treating PDFs as text in Git.
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from PIL import Image as PILImage
ROOT=Path(__file__).resolve().parents[1]
FONTS=Path('/System/Library/Fonts/Supplemental')
for n,f in [('Guide','Arial.ttf'),('GuideBold','Arial Bold.ttf')]:
    pdfmetrics.registerFont(TTFont(n,str(FONTS/f)))
pdfmetrics.registerFontFamily('Guide',normal='Guide',bold='GuideBold',italic='Guide',boldItalic='GuideBold')
GOLD=HexColor('#9e6c00');GRAY=HexColor('#595959');TEXT=HexColor('#262626')
WIDTH=481.276
styles={
 'body':ParagraphStyle('body',fontName='Guide',fontSize=9.1,leading=12.2,textColor=GRAY,spaceAfter=7),
 'h1':ParagraphStyle('h1',fontName='GuideBold',fontSize=16,leading=20,textColor=GOLD,spaceAfter=10,keepWithNext=True),
 'h2':ParagraphStyle('h2',fontName='GuideBold',fontSize=11,leading=14,textColor=GOLD,spaceBefore=8,spaceAfter=7,keepWithNext=True),
 'title':ParagraphStyle('title',fontName='GuideBold',fontSize=25,leading=29,textColor=GOLD,spaceAfter=11),
 'kicker':ParagraphStyle('kicker',fontName='GuideBold',fontSize=10,leading=12,textColor=HexColor('#ca8e01'),spaceAfter=8),
 'subtitle':ParagraphStyle('subtitle',fontName='GuideBold',fontSize=10,leading=13,textColor=GRAY,spaceAfter=12),
 'cell':ParagraphStyle('cell',fontName='Guide',fontSize=7.2,leading=9.4,textColor=TEXT),
 'thead':ParagraphStyle('thead',fontName='GuideBold',fontSize=6.8,leading=9,textColor=white),
 'small':ParagraphStyle('small',fontName='Guide',fontSize=8.4,leading=11,textColor=GRAY,spaceAfter=7),
 'note':ParagraphStyle('note',fontName='Guide',fontSize=8.4,leading=11,textColor=HexColor('#7c3e00')),
}
def P(s,style='body'):return Paragraph(s,styles[style])
def table(rows,widths):
    total=sum(widths);widths=[w*WIDTH/total for w in widths]
    t=Table([[P(str(v),'thead' if i==0 else 'cell') for v in row] for i,row in enumerate(rows)],colWidths=widths,hAlign='LEFT',repeatRows=1)
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),GOLD),('ROWBACKGROUNDS',(0,1),(-1,-1),[white,HexColor('#f5f5f5')]),('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),4),('RIGHTPADDING',(0,0),(-1,-1),4),('TOPPADDING',(0,0),(-1,-1),4),('BOTTOMPADDING',(0,0),(-1,-1),4),('GRID',(0,0),(-1,-1),.35,HexColor('#d6d6d6')),('LINEBEFORE',(1,0),(-1,0),.35,white)]))
    return t
def note(s,cover=False):
    t=Table([[P(s,'note')]],colWidths=[WIDTH],hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,-1),HexColor('#faf4e6' if cover else '#fff1df')),('BOX',(0,0),(-1,-1),.5,GOLD if cover else HexColor('#ea8a22')),('LEFTPADDING',(0,0),(-1,-1),7),('RIGHTPADDING',(0,0),(-1,-1),7),('TOPPADDING',(0,0),(-1,-1),7),('BOTTOMPADDING',(0,0),(-1,-1),7)]))
    return t
def checklist(items):return [P('□  '+s) for s in items]
def view(name,rev,side):
    path=ROOT/f'images/rain-detector/RainDetector{name}-Rev{rev}-{side}.png'
    # Crop only empty margins for the PDF; preserve all native geometry and shadows.
    im=PILImage.open(path).convert('RGBA');bbox=im.getbbox()
    import io
    stream=io.BytesIO();im.crop(bbox).save(stream,format='PNG');stream.seek(0)
    w,h=bbox[2]-bbox[0],bbox[3]-bbox[1];scale=min(WIDTH/w,360/h)
    return Image(stream,width=w*scale,height=h*scale)
def build(name,rev,part,partner):
    folder=ROOT/f'hardware/kicad/Sensors/RainDetector{name}'
    out=folder/f'Documentation/RainDetector{name}_Bestueckungs_und_Bringup_Rev{rev}.pdf'
    ishead=name=='Head'
    intro=('Passiver kapazitiver Sensorkopf mit lötstoppmaskenbedeckten Kupferelektroden und zwanzig Heizwiderständen.' if ishead else 'ICM7555-Frequenzfront-end mit 3,3-V-Ausgang und AO3400A-Schalter für die 5-V-Heizung des separaten Heads.')
    story=[Spacer(1,21),P(f'ENVNODE RAINDETECTOR {name.upper()}','kicker'),P('Bestückungs- und<br/>Bring-up-Anleitung','title'),P(f'PCB Revision {rev} · Mechanikstand 20. Oktober 2026','subtitle'),view(name,rev,'top'),Spacer(1,14),note('<b>Geltungsbereich:</b> '+intro+' Diese Anleitung beschreibt Handbestückung, Sichtprüfung und Erstinbetriebnahme mit Kleinspannung. Die Kontur von 62,12 x 54,52 mm unterstützt die Gehäuseabdichtung. C1 und J1/J2 bleiben unverändert.',True),Spacer(1,9),P(f'Dokumentstand: 21. Oktober 2026 · PCB {part} · Partner: {partner}','small')]
    story += [PageBreak(),P('1. Teile und Vorbereitung','h1'),P('1.1 Elektronische Bestückung','h2')]
    rows=[['Ref.','Anz.','Value / elektrische Daten','EnvNode-Part','Gehäuse / Footprint']]
    if ishead:
        rows += [['R1-R20','20','15 Ω, 1 %, 250 mW; WR12X15R0FTL','ENP-00115','1206; B.Cu'],['J1, J2','1','2x10-SMD-Leiste; zwei 2x2-Zuschnitte','ENP-00111','2,54 mm; B.Cu'],['PCB','1',f'RainDetectorHead Rev. {rev}',part,'62,12 x 54,52 mm']]
    else:
        rows += [['C1','1','100 nF, X7R, 100 V, 10 %','ENP-00006','1206'],['R1','1','100 Ω, 5 %, 250 mW','ENP-00017','1206'],['R2, R4','2','100 kΩ, 0,1 %, 250 mW','ENP-00016','1206'],['R3','1','10 kΩ, 0,1 %, 250 mW','ENP-00018','1206'],['Q1','1','AO3400A, N-Kanal-MOSFET','ENP-00013','SOT-23'],['U1','1','ICM7555IBAZ, CMOS-Timer','ENP-00020','SOIC-8'],['J1, J2','2','BKL 10120957, Buchsenleiste 2x2','ENP-00116','2,54 mm; THT; B.Cu'],['PCB','1',f'RainDetectorController Rev. {rev}',part,'62,12 x 54,52 mm']]
    story += [table(rows,[53,30,174,76,148]),Spacer(1,12),note('<b>Artikelstand:</b> Die Bauteilartikel stammen aus der bisherigen Bestückungsbasis. Kits und Assemblies werden separat an die neue PCB-Revision angepasst; diese Anleitung bestätigt keine aktualisierte Kit-Stückliste. Schrauben, Abstandshalter und Dichtung separat festlegen.')]
    if ishead:
        story += [Spacer(1,9),P('Menge 1 bei ENP-00111 bezeichnet eine ganze Leiste. Daraus entstehen zwei 2x2-Zuschnitte für J1/J2; Schnittverlust berücksichtigen. C1 ist gefertigte Kupferstruktur, kein zusätzliches Kauf- oder Lötbauteil.')]
    else:
        story += [Spacer(1,9),P('J3/J4 sind im Schaltplan DNP. Anschlussvariante und zugentlastete Kontaktierung separat festlegen. Ein Rendering kann DNP-Positionen zeigen; maßgeblich sind Schaltplan und Bestückungsvariante.')]
    story += [PageBreak(),P('1.2 Orientierung und Sicherheit','h2')]+checklist(['Beide Platinen zeigen mit B.Cu zueinander. Die aktiven Controller-Bauteile bleiben von innen zugänglich.','J1/J2 nicht aus der gespiegelten Unterseitenansicht ableiten. Kontaktachsen, Orientierung und reale Steckhöhe mit den gewählten Teilen prüfen.','Vier 3,2-mm-NPTH-Bohrungen für M3. Schrauben und Abstandshalter dürfen weder Kupfer noch Lötstellen berühren.','Die vergrößerte Kontur unterstützt die Dichtungsauflage. Via-Verschluss, Beschichtung und Wasserausschluss am gefertigten Aufbau prüfen.'])
    story += [note('<b>Elektrische Grenze:</b> HEATER_EN und RAIN_FREQ sind 3,3-V-Signale. Keine 5 V an den ESP32-Signaleingang oder die 3,3-V-Schiene anschließen. Zunächst nur mit strombegrenzter Kleinspannung und unter Aufsicht prüfen.'),P('1.3 Benötigte Hilfsmittel','h2')]+checklist(['Feinlötkolben, geeignetes Lot und Flussmittel; Vergrößerung für SMD-Lötstellen.','Multimeter mit Widerstands- und Durchgangsmessung; Oszilloskop oder Frequenzzähler.','Strombegrenzte 3,3-V- und 5-V-Versorgung mit gemeinsamer Masse; isolierte Prüfleitungen.','Passender Head/Controller, Temperaturmessung und mechanische Passprobe.'])
    coords=([['H1/H2','3,676955 / 58,443045','3,676955'],['H4/H3','3,676955 / 58,443045','50,843045']] if ishead else [['H1/H2','3,680 / 58,440','3,680'],['H4/H3','3,680 / 58,440','50,940']])
    story += [P('1.4 Mechanische Bezugspunkte','h2'),P('Kontur: 62,12 x 54,52 mm (Edge.Cuts-Mittellinie). Bohrungsmittelpunkte in mm ab linker/oberer Konturkante in F.Cu-Koordinaten:'),table([['Bohrungen','X links / rechts','Y']]+coords,[100,230,151]),Spacer(1,6),P('Die tatsächlichen Koordinaten gelten je PCB, nicht als gemeinsame Bohrschablone. Die unteren Y-Positionen unterscheiden sich um knapp 0,1 mm; vor mechanischer Fertigung gegen die Gehäusekonstruktion prüfen.','small')]
    story += [PageBreak(),P('2. Bestückungsübersicht','h1'),P(('Sensorseite F.Cu: C1 besteht aus zwei von Lötstopplack bedeckten, ineinandergreifenden Elektroden. Die Kupferstruktur ist kein aufzulötendes Bauteil.' if ishead else 'Bestückungsseite F.Cu: U1 erzeugt das Frequenzsignal; Q1 schaltet den Heizstrom. J3/J4 bilden die getrennten Signalanschlüsse zum Mainboard. J1/J2 sitzen auf der Unterseite.')),Spacer(1,8),view(name,rev,'top'),Spacer(1,12),note('Die native KiCad-Ansicht zeigt die aktuelle Geometrie. Sensorfläche und Interconnect-Positionen sind gegenüber dem vorherigen PCB-Stand unverändert. Reale Beschichtung und Via-Verschluss lassen sich aus dem Rendering nicht ableiten.')]
    story += [PageBreak(),P('2.1 Unterseite und Interconnects','h2'),P(('B.Cu: Zwanzig Heizwiderstände und die SMD-Stiftleisten J1/J2. Die Controller-Buchsen stehen dieser Seite gegenüber.' if ishead else 'B.Cu: Die Buchsen J1/J2 zeigen zum Head. In dieser Ansicht sind Steckerbelegung und Bestückung gegenüber der Oberseite gespiegelt.')),Spacer(1,8),view(name,rev,'bottom'),Spacer(1,12),note('<b>Wichtig:</b> Die Kontaktachsen der Steckverbinder sind maßgeblich, nicht die abwechselnden SMD-Lötpadmitten. Passung, Steckhöhe und Kontaktzuordnung vor dem Zusammenstecken mechanisch und elektrisch prüfen.')]
    story += [PageBreak(),P('3. Empfohlene Löt-Reihenfolge','h1')]
    steps=([('Widerstände identifizieren','R1-R20 als 15 Ω identifizieren. Werte anhand Messung oder belastbarer Kennzeichnung prüfen.'),('Heizstränge bestücken','Alle zwanzig Widerstände auf B.Cu montieren. Jeweils vier Widerstände eines Strangs vollständig prüfen; keine Position auslassen.'),('J1/J2 ausrichten','Zwei 2x2-Zuschnitte herstellen. Zunächst je ein Pad fixieren, Kontaktachsen und Rechtwinkligkeit prüfen, dann vollständig löten.'),('Reinigung und Sichtprüfung','Flussmittel materialverträglich entfernen. Maskierung, Via-Verschluss, Platinenkanten und Sensorfläche nicht beschädigen.')]
        if ishead else [('Passive Bauteile','C1 und R1-R4 bestücken. R1 = 100 Ω nicht mit R2/R4 = 100 kΩ verwechseln; R3 = 10 kΩ.'),('U1 und Q1','Pin 1 und Gehäusemarkierung kontrollieren. Einen Pin fixieren, Lage prüfen und alle Pins fertig löten.'),('Buchsen J1/J2','Von B.Cu montieren. Parallelität und Rechtwinkligkeit mit einer Vorrichtung prüfen; danach mechanisch belastbar verlöten.'),('Anschlussoption J3/J4','Gemäß dokumentierter Variante DNP lassen oder freigegeben bestücken. Zugentlastete Kontaktierung vorsehen.'),('Reinigung und Sichtprüfung','Flussmittelreste entfernen und alle Lötstellen unter Vergrößerung prüfen. Head erst nach Einzelprüfung aufstecken.')])
    for i,(title,body) in enumerate(steps,1):story += [P(f'{i}. {title}','h2'),P(body)]
    if not ishead:story += [note('U1.1 = GND; U1.4/U1.8 = +3V3. Q1.1 = Gate, Q1.2 = Source/GND, Q1.3 = Drain/HEATER-. Gehäusemarkierungen zusätzlich mit den verwendeten Bauteilen abgleichen.')]
    story += [PageBreak(),P('4. Anschlussbelegung','h1'),P('4.1 Verbindung zwischen Head und Controller','h2'),table([['Head','Signal','Controller'],['J1.1','HEATER+ / +5V','J1.1'],['J1.4','GND / Elektrode 2','J1.4'],['J2.1','SENSE / Elektrode 1','J2.1'],['J2.4','HEATER- / geschalteter Rückleiter','J2.4'],['J1/J2 Pins 2 und 3','Elektrisch unverbunden','Elektrisch unverbunden']],[95,291,95]),P('4.2 Mainboard-Anschlüsse des Controllers','h2'),table([['Anschluss','Pin 1','Pin 2','Pin 3','Pin 4'],['J3 HEATER_CTRL','GND','+5V','HEATER_EN','+3V3'],['J4 RAIN_SIGNAL','GND','+5V','RAIN_FREQ','+3V3']],[120,70,70,151,70]),Spacer(1,8),P('Die Versorgungspins sind gemeinsam geführt. Keine unabhängigen Netzteile parallel verbinden. HEATER- ist kein dauerhaftes GND; eine externe Verbindung nach Masse umgeht die Abschaltung durch Q1.'),P('4.3 Heiznetzwerk und Sensorfläche','h2'),P('Fünf parallele Stränge mit je vier 15-Ω-Widerständen: R1-R4, R5-R8, R9-R12, R13-R16, R17-R20. Je Strang 60 Ω, gesamt 12 Ω. Bei 5 V nominal 417 mA und 2,08 W; etwa 0,104 W pro Widerstand. Mindestens 0,25 W einsetzen und Temperatur-Derating beachten. Dies sind Rechenwerte.'),P('C1-Footprint: RainDetector_Interdigitated_44.78x37.25mm_P0.51mm. Keine F.Mask-/F.Paste-Öffnungen über den Sensorpads. Fläche nicht verzinnen, freilegen oder mit Lötpaste versehen. 0,10 mm Mindestabstand gilt nur für C1.'),note('Via-Plugging und vorderseitiges Tenting mit dem Fertiger abstimmen. Filling/Capping sind nicht als Type VII freigegeben. Maskententing allein ist kein Nachweis der Abdichtung.')]
    checks=(['Heizpfad J1.1 nach J2.4 nominal 12 Ω; ein offener Strang ergibt rechnerisch 15 Ω.','J2.1 nach C1.1 (SENSE) und J1.4 nach C1.2 (GND-Elektrode) prüfen.','Kein niederohmiger Kurzschluss zwischen SENSE und GND oder zwischen Heizpfad und Elektroden bei getrenntem Controller.','Pins 2/3 unverbunden; Maske intakt; Via-Verschluss, Steckhöhe und Isolierung geprüft.'] if ishead else ['Kein Kurzschluss zwischen +3V3, +5V und GND; U1/Q1 korrekt orientiert.','J3.1/J4.1/Q1.2 an GND; J3.2/J4.2 an J1.1 (+5V).','J3.4/J4.4/C1.1/U1.4/U1.8 an +3V3; C1.2 an GND.','J3.3 über R1 = 100 Ω zum Gate; R2 = 100 kΩ von Gate nach GND.','Q1.3 nach J2.4; J2.1 nach U1.2/U1.6; J4.3 nach U1.3.','Pins 2/3 von J1/J2 elektrisch unverbunden.'])
    story += [PageBreak(),P('5. Prüfungen ohne Versorgung','h1')]+checklist(checks)
    story += [P('6. Erstinbetriebnahme','h1'),table([['Schritt','Aktion / Erwartung'],['1','5-V-Heizversorgung aus; HEATER_EN hardwareseitig LOW halten. Head zunächst getrennt.'],['2','Controller mit strombegrenzten 3,3 V versorgen. 20 mA sind ein vorsichtiger Startwert, kein spezifizierter Sollstrom. Bei Begrenzung oder Erwärmung abschalten.'],['3','3,3 V an U1.4/U1.8 und C1 kontrollieren; Ruhestrom erfassen.'],['4','Ausschalten, Head richtig aufstecken, 3,3 V wieder einschalten.'],['5','RAIN_FREQ gegen GND mit Oszilloskop/Frequenzzähler prüfen. Ohne Head ist keine definierte Grundfrequenz zugesichert.']],[45,436]),Spacer(1,10),note('Trockene Grundfrequenz bei festgelegter Montage, Beschichtung und Temperatur erfassen. Kontrollierte Benetzung darf Controller und Anschlüsse nicht erreichen. Frequenzänderung und Rückkehr dokumentieren.')]
    story += [PageBreak(),P('6.1 Funktions- und Heiztest','h2')]+checklist(['5 V mit zunächst etwa 0,5 A Strombegrenzung zuschalten; Aufbau beaufsichtigen.','HEATER_EN LOW: Heizung aus. HIGH mit 3,3 V: nominal 417 mA zusätzlich zu Nebenlasten.','Versorgungseinbruch und Temperaturen an Q1, Head und Kontakten prüfen; bei Auffälligkeiten abschalten.','Wieder LOW: Heizstrom muss zurückgehen. Steuersignal trennen und Ausschaltung prüfen.'])
    story += [P('7. Messprotokoll','h1'),table([['Prüfpunkt','Soll / Erwartung','Messwert','Bestanden','Bemerkung'],['+3V3; +5V unter Last','3,3 V; 5 V nominal','','',''],['Ruhestrom ohne Heizung','Dokumentieren','','',''],['Heizwiderstand kalt','Nominal 12 Ω','','',''],['Frequenz trocken / benetzt','Grundwert und Änderung','','',''],['Heizstrom bei 5 V','Ca. 417 mA + Nebenlasten','','',''],['HEATER_EN LOW / offen','Heizung aus','','',''],['Q1 / Head / Kontakte','Temperatur und Dauer','','',''],['Trocken / Signalverlust','Abschaltung prüfen','','',''],['Dichtung / Beschichtung','Separat qualifizieren','','','']],[115,130,70,60,106]),P('7.1 Firmwarestand','h2'),P('Aktivierte RainDetector-Sensoren und -Controller starten nach Boot mit Heizung aus. Neue Benetzung fordert 100 % an. Regenende folgt voreingestellt nach 60 s ohne neue erkannte Benetzung; Heizung bleibt bis bestätigter Trockenfrequenz an. 32/34 kHz sind einstellbar. 5000 ms betrifft die Veröffentlichung, lokale Erfassung etwa 250 ms. Keine Temperatur-/Taupunktregelung oder SD-Feldlogs in diesem Stand. Details: docs/RainDetector.md.','small')]
    story += [PageBreak(),P('8. Fehlerbilder','h1'),table([['Beobachtung','Zuerst prüfen'],['Keine Frequenz','3,3 V, U1-Orientierung, R3/R4, SENSE-Kontakt und Head.'],['Heizung bleibt aus','5 V, Enable-Pegel, R1, Q1 und Heizpfad.'],['Heizung bleibt an','Steuersignal trennen; R2, Gate-Brücken und Q1 prüfen.'],['Instabile Frequenz','Versorgung, Kontakte, Feuchtigkeit, Montage und Referenzmessgerät.'],['Ungleichmäßige Erwärmung','Abschalten; Widerstandswerte, Stränge, Lötstellen und Versorgung prüfen.']],[125,356]),P('9. Revisionsnachweis','h1')]
    sources=[['Quelle','SHA-256']]
    for suffix in ['kicad_pcb','kicad_sch','kicad_pro']:
        source=folder/f'RainDetector{name}.{suffix}';sources.append([source.name,hashlib.sha256(source.read_bytes()).hexdigest()])
    story += [table(sources,[155,326]),Spacer(1,10),P('Prüfumfang 21.10.2026: Kontur, Bohrungen, unveränderte C1-/J1-/J2-Lage und Pad-Netzzuordnungen wurden gegen den vor der Mechanikänderung committed PCB-Stand verglichen. Die Laborergebnisse betreffen den bisherigen Aufbau. Kein neuer vollständiger ERC/DRC-Lauf, keine Fertigungsfreigabe und kein physischer Test der neuen mechanischen Revision.','small'),P('Native Renderings: KiCad 10.0.5; oben -25/0/5 Grad, unten -25/0/-5 Grad, perspektivisch. Nicht modellierte Gehäuse und reale Beschichtungen werden nicht nachgebildet. Layoutvorlage: DuoRelay, Bestückungs- und Bring-up-Anleitung Rev. 0.3.','small'),P('EnvNode-eigene Inhalte: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe LICENSE im Repository. Drittanbieter-Modelle behalten ihre Lizenzen.','small')]
    def footer(c,doc):
        c.setFont('GuideBold',6.8);c.setFillColor(GOLD);c.drawString(51,807.89,f'ENVNODE  ·  RAINDETECTOR {name.upper()} REV. {rev}')
        c.setFont('Guide',6.8);c.setFillColor(GRAY);c.drawRightString(544.276,28.3,f'Bestückungs- und Bring-up-Anleitung  ·  Entwurf  ·  {doc.page}')
    SimpleDocTemplate(str(out),pagesize=(595.276,841.89),leftMargin=51,rightMargin=51,topMargin=51,bottomMargin=45,title=f'RainDetector{name} Rev. {rev}',author='EnvNode').build(story,onFirstPage=footer,onLaterPages=footer)
    print(out)
if __name__=='__main__':
    build('Head','0.2','ENP-00139','Controller 0.4')
    build('Controller','0.4','ENP-00138','Head 0.2')
