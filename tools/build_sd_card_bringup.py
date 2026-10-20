#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../LICENSE. Adafruit and KiCad model licenses remain unchanged.
"""Build the SD-Card Rev. 0.1 guide in the existing EnvNode bring-up style.
Requires reportlab and Pillow. Run from any directory. KiCad images must match
images/manifest.json; regenerate and review all images after changing the PCB.
"""
from pathlib import Path
from hashlib import sha256
import json
from xml.sax.saxutils import escape
from PIL import Image as PILImage
import reportlab
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.pdfgen import canvas
from reportlab.lib.colors import HexColor, white, black
from reportlab.lib.styles import ParagraphStyle
from reportlab.platypus import Paragraph, Table, TableStyle
from reportlab.lib.utils import ImageReader

ROOT = Path(__file__).resolve().parents[1]
PROJECT = ROOT / 'hardware/kicad/Modules/FullSize/SD-Card'
DOC = PROJECT / 'Documentation'
IMAGES = DOC / 'images'
OUTPUT = DOC / 'SD_Card_Bestueckungs_und_Bringup_Rev0.1.pdf'
W, H = 595.276, 841.89
LEFT, RIGHT, WIDTH = 50, 545.276, 495.276
AMBER, LIGHT, GREY = HexColor('#a87300'), HexColor('#fff6e5'), HexColor('#5b5b5b')
# Embed fonts for consistent rendering without machine-specific PDF substitutions.
font_dir = Path('/System/Library/Fonts/Supplemental')
if (font_dir / 'Arial.ttf').exists():
    font_files = ['Arial.ttf', 'Arial Bold.ttf', 'Courier New.ttf']
else:
    font_dir = Path(reportlab.__file__).parent / 'fonts'
    font_files = ['Vera.ttf', 'VeraBd.ttf', 'VeraMono.ttf']
for font_name, filename in zip(['Guide', 'GuideBold', 'GuideMono'], font_files):
    pdfmetrics.registerFont(TTFont(font_name, str(font_dir / filename)))
pdfmetrics.registerFontFamily('Guide', normal='Guide', bold='GuideBold', italic='Guide', boldItalic='GuideBold')
BODY = ParagraphStyle('body', fontName='Guide', fontSize=9.5, leading=13,
                      textColor=black, spaceAfter=8)
SMALL = ParagraphStyle('small', parent=BODY, fontSize=8, leading=10.5)
CELL = ParagraphStyle('cell', parent=BODY, fontSize=8, leading=10)
HEAD = ParagraphStyle('head', parent=CELL, fontName='GuideBold', textColor=white)

def digest(path):
    return sha256(path.read_bytes()).hexdigest()

manifest = json.loads((IMAGES / 'manifest.json').read_text())
assert manifest['pcb_sha256'] == digest(PROJECT / 'SD-Card.kicad_pcb'), 'Rerender/review PCB images'
for name, expected in manifest['images'].items():
    assert digest(IMAGES / name) == expected, f'Changed image: {name}'
assert 'Found 0 DRC violations' in (DOC / 'SD_Card_DRC_Rev0.1.txt').read_text()

c = canvas.Canvas(str(OUTPUT), pagesize=(W, H), pageCompression=1)
c.setTitle('EnvNode SD-Card Rev. 0.1 - Bestückungs- und Bring-up-Anleitung')
c.setAuthor('EnvNode')
c.setSubject('Erstbestückung und Hardware-Bring-up; InvenTree-BOM ENK-00022 angelegt; physischer Bring-up und Firmwareintegration offen')
page = 0
y = 0

def start(title=None):
    global page, y
    page += 1
    c.setFillColor(AMBER); c.setFont('GuideBold', 7)
    c.drawString(LEFT, 808, 'ENVNODE  ·  SD-CARD REV. 0.1')
    y = 765
    if title:
        c.setFont('GuideBold', 17); c.drawString(LEFT, y, title); y -= 29

def end():
    assert y >= 56, f'Page {page} overflow: {y}'
    c.setFillColor(GREY); c.setFont('Guide', 6.5)
    c.drawRightString(RIGHT, 29, f'Bestückungs- und Bring-up-Anleitung  ·  Entwurf  ·  {page}')
    c.showPage()

def sub(text):
    global y
    y -= 6
    c.setFillColor(AMBER); c.setFont('GuideBold', 11)
    c.drawString(LEFT, y, text); y -= 20

def para(text, small=False):
    global y
    p = Paragraph(text, SMALL if small else BODY)
    _, h = p.wrap(WIDTH, H); p.drawOn(c, LEFT, y-h); y -= h+9

def box(text):
    global y
    p = Paragraph(text, SMALL); _, h = p.wrap(WIDTH-16, H)
    c.setFillColor(LIGHT); c.setStrokeColor(AMBER); c.setLineWidth(.6)
    c.rect(LEFT, y-h-16, WIDTH, h+16, fill=1, stroke=1)
    p.drawOn(c, LEFT+8, y-h-8); y -= h+28

def check(text):
    global y
    p = Paragraph(text, BODY); _, h = p.wrap(WIDTH-17, H)
    c.setStrokeColor(GREY); c.setLineWidth(.6); c.rect(LEFT+1,y-8,6,6,fill=0,stroke=1)
    p.drawOn(c,LEFT+17,y-h); y -= max(h,10)+9

def table(headers, rows, widths):
    global y
    data = [[Paragraph(escape(str(v)), HEAD) for v in headers]]
    data += [[Paragraph(escape(str(v)),CELL) for v in row] for row in rows]
    t = Table(data,colWidths=widths,hAlign='LEFT')
    t.setStyle(TableStyle([('BACKGROUND',(0,0),(-1,0),AMBER),
        ('ROWBACKGROUNDS',(0,1),(-1,-1),[white,HexColor('#f2f2f2')]),
        ('GRID',(0,0),(-1,-1),.3,HexColor('#bcbcbc')),
        ('VALIGN',(0,0),(-1,-1),'TOP'),('LEFTPADDING',(0,0),(-1,-1),5),
        ('RIGHTPADDING',(0,0),(-1,-1),5),('TOPPADDING',(0,0),(-1,-1),5),
        ('BOTTOMPADDING',(0,0),(-1,-1),5)]))
    _,h=t.wrap(WIDTH,H);t.drawOn(c,LEFT,y-h); y-=h+14

def picture(name, height, caption):
    global y
    im=PILImage.open(IMAGES/name);iw,ih=im.size
    width=min(WIDTH, height*iw/ih); actual_h=width*ih/iw
    c.drawImage(ImageReader(im),LEFT+(WIDTH-width)/2,y-actual_h,width=width,height=actual_h)
    y-=actual_h+10;para(caption,small=True)

start()
c.setFillColor(AMBER);c.setFont('GuideBold',10);c.drawString(LEFT,749,'ENVNODE SD-CARD')
c.setFont('GuideBold',26);c.drawString(LEFT,715,'Bestückungs- und');c.drawString(LEFT,684,'Bring-up-Anleitung')
c.setFillColor(GREY);c.setFont('GuideBold',10)
c.drawString(LEFT,660,'FullSize-Modul Revision 0.1 · Erster Entwurf · PCB-Stand 20. Oktober 2026')
y=642
picture('oblique.png',365,'KiCad-Ansicht der Trägerplatine; das Adafruit-Breakout U1 ist nicht als 3D-Modell dargestellt.')
box('Geltungsbereich: Handbestückung, Sichtprüfung und Erstinbetriebnahme der 38 x 64 mm großen '
    'Trägerplatine mit Adafruit 254. Optionales Diagnosemodul zur Feldvalidierung des Raindetectors. '
    'Eine integrierte SD-Aufzeichnungsfunktion ist noch nicht implementiert.')
para('Dokumentstand: 20. Oktober 2026',True)
para('EnvNode-eigene Inhalte: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe LICENSE im Repository. '
     'Adafruit-Design und Adapter: CC BY-SA 3.0, Limor Fried/Ladyada für Adafruit Industries. '
     'KiCad-Standardmodelle behalten ihre jeweiligen Lizenzhinweise.',True)
end()

start('1. Teile und Vorbereitung');sub('1.1 Elektronische Bestückung')
table(['Ref.','Anz.','Value / elektrische Daten','Artikelzuordnung','Gehäuse / Footprint'],[
 ['U1','1','Adafruit MicroSD card breakout board+, Produkt 254','ENP-00136; Reichelt DEBO MICROSD','Fertiges Breakout; 1x8 / 2,54 mm'],
 ['U2','1','24LC32 / 24LC32A; 3,3-V-Betrieb','ENP-00102','SOIC-8, 3,9 x 4,9 mm'],
 ['C1','1','100 nF, X7R; für 3,3 V geeignet','ENP-00006','1206 / 3216 Metric'],
 ['R1','1','10 kOhm, CD-Pull-up','ENP-00018','1206 / 3216 Metric'],
 ['J1','1','Stiftleiste 2x7, 2,54 mm, Bottom-Mount','ENP-00059','EnvNode Bottom-Footprint'],
 ['PCB','1','SD-Card FullSize Rev. 0.1','ENP-00137','38,00 x 64,00 mm'],
 ['U1-Pins','1','Gerade Stiftleiste 1x8, direkt eingelötet','Bei U1 enthalten; ENP-00019 nur Ersatz','2,54 mm, 1,0-mm-Bohrungen'],
 ['Mechanik','n. B.','Isolierende Distanzstücke, Schrauben / Muttern','Nicht in BOM; erst prüfen','H1/H2: M2,5; U1: #2 / 2,2 mm'],
 ['Speicher','1','Geeignete microSD; für ersten Test FAT32','Karte dokumentieren','Keine Karte im Lieferumfang voraussetzen']
],[55,28,177,100,WIDTH-360])
box('<b>InvenTree-BOM: ENK-00022, angelegt und validiert.</b> Sechs Positionen, jeweils Menge 1: '
    'PCB, U1, U2, C1, R1 und J1. Distanzhülsen und Montagematerial sind auf Nutzerwunsch '
    'bis zur Prüfung ausdrücklich nicht in Bestand/BOM aufgenommen. microSD separat.')
para('Direkte Montage über die laut Adafruit mitgelieferte 1x8-Stiftleiste; keine zusätzliche Buchse. '
     'ENP-00019 ist kein zusätzlicher BOM-Posten. Lieferumfang vor dem Aufbau prüfen. '
     'BOM: inventree.thielemann.intern/web/part/135/bom')
para('Modulidentifikation: U2 wird aus +3V3_SYS versorgt. I2C0-Adresse 0x52 in Slot A bzw. 0x53 in Slot B. '
     'Ein SD-Card-spezifischer Descriptor und seine Firmwareunterstützung sind noch offen; keine fremde Modulvorlage verwenden.')
end()

start('1.2 Orientierung und Handhabung')
for t in ['J1 wird auf der <b>Unterseite</b> bestückt. Pin 1 anhand des quadratischen Pads und des Bottom-Footprints prüfen; nicht spiegeln.',
 'U2 und C1 liegen auf der Unterseite; R1 auf der Oberseite. U2 Pin 1 mit dem Footprint abgleichen.',
 'U1 sitzt auf der Oberseite, Bauteile und Kartensockel nach oben. U1 Pin 1 ist der 5-V-Eingang, nicht GND.',
 'U1.2 ist der Ausgang des Adafruit-Reglers. Nicht mit +3V3_SYS oder einer externen 3,3-V-Quelle verbinden.',
 'Die H1-Überdeckung ist gewollt. H1-Befestigung vor U1 montieren; Schraubenkopf, Abstandshalter und Unterseite des Breakouts dürfen sich nicht berühren.',
 'U1 besitzt vier 2,2-mm-Bohrungen. M2,5-Schrauben nicht mit Kraft einsetzen. H1/H2 der Trägerplatine sind 2,7-mm-Bohrungen für M2,5.',
 'Modul nur spannungsfrei stecken/abziehen. Karte beim ersten Bring-up ebenfalls nur bei abgeschaltetem Gerät wechseln.']:
 check(t)
sub('1.3 Benötigte Hilfsmittel')
for t in ['Lötstation, Flussmittel, Lupe, Multimeter und ESD-gerechter Arbeitsplatz.',
 'Geeignetes EnvNode-Mainboard oder Prüfadapter mit geregelten 5 V und 3,3 V, gemeinsamer Masse und Strombegrenzung.',
 'Testfirmware mit I2C-Scan, GPIO-Eingang für CD und SD-over-SPI-Dateisystemtest; serielle Konsole.',
 'Bekannte, entbehrliche FAT32-Testkarte und Kartenleser; optional Oszilloskop/Logikanalysator für Versorgung und SPI.']:
 check(t)
box('Der Karteninhalt kann beim Schreibtest verändert werden. Testkarte ohne wichtige Daten verwenden. '
    'Ein erfolgreiches Mounten oder ein I2C-ACK ersetzt keine Prüfung von Datenintegrität und Modulidentität.')
end()

start('2. Bestückungsübersicht')
para('Oberseite: J1 liegt oben. R1 befindet sich links oberhalb der U1-Anschlussreihe. '
     'U1 Pin 1 liegt in dieser Ansicht rechts. Der Kartenzugang liegt am unteren Ende des U1-Umrisses, Richtung H2.')
picture('top.png',435,'Draufsicht aus dem gespeicherten PCB. Weißer U1-Umriss = Breakout-Position; '
        'U1 selbst ist wegen des fehlenden 3D-Modells nicht sichtbar. H1 liegt innerhalb dieses Umrisses.')
box('Vor dem Löten U1 probeweise mit Stiftleiste und Abstandshaltern einsetzen. '
    'Alle vier Lochpositionen, Pinfolge, Zugang zum Push-Push-Sockel und Freiraum über H1 prüfen. '
    'Der 10-mm-Zugangsbereich im Footprint ist eine Planungshilfe, keine garantierte Fingerfreiheit.')
end()

start('2.1 Unterseite und Modulstecker')
picture('bottom.png',440,'Unterseitenansicht: J1 rechts, U2 und C1 links oben. Die gegenüber der Oberseite '
        'gespiegelte Darstellung ist beabsichtigt; Pinbelegung immer über Pinnummern zuordnen.')
box('<b>Bottom-Mount:</b> J1 muss zur Buchse auf dem Mainboard passen. Lötseite und Steckrichtung '
    'gegen das tatsächliche Board prüfen. Ein gewöhnlicher von oben bestückter Header ist kein pauschaler Ersatz.')
para('U2 Pin 1 (A0) folgt EEPROM_A0. Pin 2 (A1) liegt an +3V3_SYS, Pin 3 (A2) an GND. '
     'Pin 4 = GND, Pin 5 = SDA, Pin 6 = SCL, Pin 7 = WP/GND und Pin 8 = +3V3_SYS. '
     'C1 liegt zwischen Pin 8-Versorgung und GND.',True)
end()

start('3. Empfohlene Löt-Reihenfolge')
for title,txt in [
 ('1. Passivteile','C1 = 100 nF auf der Unterseite, R1 = 10 kOhm auf der Oberseite bestücken. Werte und Lötbrücken kontrollieren.'),
 ('2. EEPROM U2','Pin 1 ausrichten; zunächst einen Pin fixieren, anschließend alle SOIC-8-Pins löten. Keine Brücken an SDA/SCL oder Versorgung.'),
 ('3. Bottom-Mount J1','Von der Unterseite montieren. Mit geeigneter Gegenbuchse rechtwinklig ausrichten, zwei Pins fixieren, Sitz prüfen und fertig löten.'),
 ('4. Mechanik vorbereiten','H1-Befestigung vor U1 einsetzen. Spacerhöhe nach realem Steckverbinder festlegen. H2 und die U1-Stützen auf spannungsfreien Sitz prüfen.'),
 ('5. U1-Anschluss bestücken','1x8-Stiftleiste zur direkten Lötmontage ausrichten. Pin 1 der Trägerplatine mit 5V am Breakout abgleichen. Keine Reihe um einen Pin versetzen.'),
 ('6. Breakout U1 montieren','Bauteilseite nach oben, Kartenzugang Richtung H2. Vor dem festen Einlöten alle später verdeckten Stellen prüfen.'),
 ('7. Reinigung und Abnahme','Lötstellen reinigen und unter Vergrößerung prüfen. Keine leitenden Reste unter U1. H1 darf die Breakout-Unterseite nicht berühren.')]:
 sub(title);para(txt)
end()

start('4. Anschlussbelegung');sub('4.1 EnvNode-Modulstecker J1')
table(['Pin','Netz','Verwendung'],[
 ['1','GND','Gemeinsame Masse'],['2','+3V3_SYS','EEPROM U2, C1 und CD-Pull-up R1'],
 ['3','EEPROM_A0','Slotabhängiges Adressbit von U2'],['4','+5V','Versorgung von U1.1'],
 ['5 / 6','I2C0 SDA / SCL','Modulidentifikation U2'],['7 / 8','I2C1 SDA / SCL','Nicht verwendet'],
 ['9','AUX_GPIO1','CD; Eingang mit R1-Pull-up nach 3,3 V'],['10','AUX_GPIO2','Nicht verwendet'],
 ['11','SPI_MOSI','U1.6 / DI'],['12','SPI_MISO','U1.5 / DO'],
 ['13','SPI_SCK','U1.4 / CLK'],['14','SPI_CS','U1.7 / CS; slotabhängig']],[50,125,WIDTH-175])
sub('4.2 Adafruit-Breakout U1')
table(['Pin','Beschriftung / Symbol','Verbindung'],[
 ['1','5V / 5V_IN','J1.4'],['2','3V / 3V3_REG','Offen lassen (Reglerausgang)'],
 ['3','GND','J1.1'],['4','CLK','J1.13'],['5','DO / MISO','J1.12'],
 ['6','DI / MOSI','J1.11'],['7','CS, aktiv LOW','J1.14'],['8','CD','J1.9, R1 = 10 kOhm nach J1.2']],[50,180,WIDTH-230])
para('Mini/Weather laut Schnittstellenspezifikation: MOSI = GPIO23, MISO = GPIO19, SCK = GPIO18. '
     'Slot A: CS = GPIO5, CD = GPIO33; Mini Slot B: CS = GPIO27, CD = GPIO32. '
     'Das tatsächliche Boardprofil muss diese Zuordnung bestätigen.',True)
end()

start('5. Prüfungen ohne Versorgung')
for t in ['Kein niederohmiger Kurzschluss zwischen J1.4 (+5 V) und GND sowie J1.2 (+3V3_SYS) und GND. Kondensator-Ladeeffekte berücksichtigen.',
 'Durchgang von J1.4 zu U1.1 und J1.1 zu U1.3. Keine direkte Kupferverbindung von U1.2 zu J1.2.',
 'J1.11/12/13/14 zu U1.6/5/4/7 einzeln messen; keine Vertauschung oder Brücke zwischen SPI-Leitungen.',
 'J1.9 zu U1.8 und R1 messen. R1 verbindet CD über 10 kOhm mit +3V3_SYS; nicht mit 5 V.',
 'U2.5 zu J1.5, U2.6 zu J1.6, U2.8/C1.1 zu J1.2; U2.4/U2.7/C1.2 zu GND.',
 'U2.1 zu J1.3, U2.2 zu +3V3_SYS und U2.3 zu GND. U2 Pin 1, J1-Lage und H1-Abstand geprüft.']:
 check(t)
sub('6. Erstinbetriebnahme')
table(['Schritt','Aktion / Erwartung'],[
 ['1','Ohne Karte starten. Modul spannungsfrei aufstecken; beide Versorgungsschienen über Mainboard oder Prüfadapter bereitstellen.'],
 ['2','Strombegrenzung zunächst vorsichtig wählen; Ströme und Spannungen erfassen. Bei Erwärmung oder Kurzschlussanzeichen sofort abschalten.'],
 ['3','J1.4 gegen GND: nominal 5 V; J1.2 und U2.8: nominal 3,3 V. U1.2 nur hochohmig messen: nahe 3,3 V. Nicht extern speisen.'],
 ['4','CD als Eingang lesen: ohne Karte laut Adafruit LOW. Ausschalten, Karte einsetzen, einschalten; mit Karte HIGH. Tatsächliche Polarität protokollieren.'],
 ['5','I2C0 scannen: U2 bei 0x52 (Slot A) bzw. 0x53 (Slot B). ACK dokumentieren; Descriptorprüfung erst mit passender SD-Card-Unterstützung.'],
 ['6','Mit Testfirmware SD-SPI initialisieren und Karte mounten. Versorgung unter Schreiblast beobachten; Stromlimit muss die gewählte Karte und das Mainboard tragen.']],[43,WIDTH-43])
end()

start('6.1 Funktions- und Belastungstest')
for t in ['Firmwarevoraussetzung: Der produktive SD-Logger ist noch nicht vorhanden. Den folgenden Test mit explizit konfigurierter SD-SPI-Testfirmware ausführen.',
 'Zunächst allein auf dem SPI-Bus testen. CS im Ruhezustand HIGH; CD ausschließlich als Eingang betreiben. SPI-Signale sind 3,3-V-Logik.',
 'Eine Testdatei mit bekanntem Inhalt schreiben, synchronisieren/schließen und bytegenau zurücklesen. Dateiname, Datenmenge, Fehlercode und Dauer notieren.',
 'Nach sauberem Schließen und Abschalten Karte am PC lesen und Inhalt vergleichen. Mehrere Kaltstarts und erneutes Mounten testen.',
 'Danach gemeinsamen SPI-Bus testen: alle anderen CS inaktiv halten; die SD-Karte nach den Vorgaben des SD-SPI-Treibers initialisieren. Andere Geräte dürfen MISO nicht blockieren.',
 'Kartenfehler, volle Karte und fehlende Karte mit der vorgesehenen Logger-Firmware gezielt testen. Messung, MQTT und Heizsteuerung müssen weiterlaufen.',
 'Zeitstempelquelle, Schreibintervall, Puffergröße und Synchronisationsintervall vor Feldbetrieb festlegen. Stromverlust kann noch nicht gesicherte Daten verlieren.']:
 check(t)
sub('7. Messprotokoll')
table(['Prüfpunkt','Soll / Eintrag','Messwert / bestanden'],[
 ['J1.4 / U1.1','5 V nominal',''],['J1.2 / U2.8','3,3 V nominal',''],
 ['U1.2, nur messen','Nahe 3,3 V; nicht verbunden',''],['CD ohne / mit Karte','LOW / HIGH; Polarität bestätigen',''],
 ['U2 I2C0-Adresse','0x52 A / 0x53 B',''],['Strom ohne / mit Schreiben','Dokumentieren; kein Spannungseinbruch',''],
 ['Schreiben / Rücklesen','Bytegenau, keine Fehler',''],['Kaltstart / Mount','Wiederholbar erfolgreich','']],[137,185,WIDTH-322])
end()

start('8. Fehlerbilder')
table(['Beobachtung','Zuerst prüfen'],[
 ['U1 ohne Versorgung','5 V an J1.4 und U1.1, Headerlage, Lötstellen und gemeinsame Masse.'],
 ['3,3-V-Systemschiene auffällig','U1.2 muss offen bleiben. U2-Pinlage, C1 und Versorgung prüfen.'],
 ['Karte wird nicht gemountet','CLK/MOSI/MISO/CS, Steckrichtung, CS-Zuordnung, FAT32-Testkarte und Versorgung.'],
 ['Schreiben bricht ab / Reset','Versorgungsspitzen messen, Stromlimit, Karte, Lötstellen und Schreibpuffer prüfen.'],
 ['CD bleibt unverändert','R1 = 10 kOhm, CD nach AUX_GPIO1, GPIO als Eingang, tatsächlichen Sockelkontakt prüfen.'],
 ['U2 antwortet nicht','3,3 V, I2C0, Mainboard-Pull-ups, A0/A1/A2 und Adresse 0x52/0x53 prüfen.'],
 ['I2C-ACK, Modul unbekannt','ACK bestätigt keine Identität. Dedizierter SD-Card-Descriptor/Firmwareunterstützung noch offen.'],
 ['Fehler mit zweitem SPI-Gerät','Individuelle CS-Leitungen, Businitialisierung, MISO-Freigabe und Treiber-Arbitrierung.'],
 ['Datei nach Ausschalten beschädigt','Sauber schließen/synchronisieren. Zeitpunkt des Stromverlusts und Datenverlustgrenze prüfen.']],[150,WIDTH-150])
sub('9. Revisionsnachweis')
para('Prüfbasis: SD-Card FullSize Rev. 0.1, PCB-Titelblock vom 20. Oktober 2026. '
     'Quellen: gespeicherter Schaltplan, PCB und Projektkonfiguration; Adafruit CAD-Stand '
     'ef4cb5171b78b6e17f12aa1fd1df41dc3f50418c. SHA-256 dient dem Nachweis des hier dokumentierten Stands.',True)
for name in ['SD-Card.kicad_pcb','SD-Card.kicad_sch','SD-Card.kicad_pro','README.md']:
 para(f'<b>{name}</b><br/><font face="GuideMono" size="6.5">{digest(PROJECT/name)}</font>',True)
end()

start('9.1 Prüfung und offene Arbeiten')
para('CAD-Prüfung am 20. Oktober 2026 mit KiCad 10.0.5: DRC einschließlich Neufüllen der '
     'Kupferflächen und Schaltplanabgleich: <b>0 Verstöße, 0 offene Verbindungen, 0 Footprintfehler '
     'und 0 Schaltplanabweichungen.</b> ERC: <b>0 Fehler, 6 Warnungen.</b>')
para('Die ERC-Warnungen betreffen I2C1_SDA, I2C1_SCL und AUX_GPIO2 als ungenutzte Schnittstellenlabels '
     'sowie die gleichnamigen lokalen/globalen Labels für +3V3_SYS, I2C0_SDA und I2C0_SCL. '
     'Die vollständigen Berichte liegen neben dieser Anleitung. Dort aufgeführte deaktivierte '
     'Prüfungen bleiben Bestandteil der dokumentierten Prüfkonfiguration.')
para('Die Überdeckung von H1 mit U1 ist eine bewusste mechanische Entscheidung. Die Bauhöhe '
     'ist im realen Aufbau zu prüfen. Renderings zeigen die Trägerplatine; ein Modell des '
     'Adafruit-Breakouts fehlt. Ein CAD-Prüflauf bestätigt keine mechanische Passprobe oder Feldqualifikation.')
sub('Offene Arbeiten vor produktiver Feldaufzeichnung')
for t in ['<b>InvenTree:</b> ENK-00022 und seine sechs BOM-Positionen sind angelegt und validiert. Bestellte Distanzhülsen erst nach Prüfung durch den Nutzer in Bestand/BOM aufnehmen; Adafruit-Herstellerverknüpfung noch ergänzen.',
 'Dedizierten Modul-Descriptor und SD-Card-Firmwarepfad definieren; keine Identität eines anderen Moduls übernehmen.',
 'Schreib-/Lesetest und Messprotokoll an der bestückten Hardware abschließen.',
 'Asynchrone Aufzeichnung, Export, Fehlerbehandlung und weiterlaufende Mess-/Heizfunktionen implementieren und testen.',
 'Geeignete Karte, Strombudget, Zeitbasis, Gehäusezugang und Schutz vor Feuchtigkeit/Kondensat im Außeneinsatz qualifizieren.']:
 check(t)
sub('Quellen und Rechte')
para('Hersteller: adafruit.com/product/254<br/>CAD/Schaltplan: '
     'learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/download<br/>'
     'CD-Kontakt: learn.adafruit.com/adafruit-micro-sd-breakout-board-card-tutorial/arduino-wiring',True)
para('EnvNode-eigene Dokumentation: LicenseRef-EnvNode-Use-and-Redistribution-1.0; siehe LICENSE. '
     'Adafruit: Designed by Limor Fried/Ladyada for Adafruit Industries, CC BY-SA 3.0; '
     'vollständige Herstellerhinweise unter Libraries/Adafruit-Breakouts. KiCad-Standardmodelle '
     'werden nicht mit dieser Anleitung neu lizenziert.',True)
box('Freigabestatus: CAD-geprüfter erster Hardwarestand. Physischer Bring-up und Feldtest '
    'sind noch durchzuführen. Kein Merge nach main im Rahmen dieser Dokumenterstellung.')
end()
assert page == 11
c.save()
print(OUTPUT)
