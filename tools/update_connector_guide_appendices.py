#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../LICENSE. Original third-party model notices remain in the image directory.
"""Rebuild the two-page connector supplement without altering historical PDF pages.
Requires reportlab and pypdf. Run from any directory; reruns replace the supplement.
"""
from io import BytesIO
from pathlib import Path
from pypdf import PdfReader, PdfWriter
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.lib.colors import HexColor
from reportlab.platypus import Paragraph
from reportlab.lib.styles import ParagraphStyle

ROOT = Path(__file__).resolve().parents[1]
IMAGES = ROOT / 'images/envnode-mini/connectors'
MARKER = 'ANSCHLUSSREFERENZ / ERGÄNZUNG'
FONT_DIR = Path('/System/Library/Fonts/Supplemental')
for name, file in [('Guide', 'Arial.ttf'), ('GuideBold', 'Arial Bold.ttf')]:
    pdfmetrics.registerFont(TTFont(name, str(FONT_DIR / file)))
TEXT = HexColor('#243746')
MUTED = HexColor('#526779')
STYLE = ParagraphStyle('body', fontName='Guide', fontSize=10, leading=14, textColor=TEXT)


def paragraph(c, text, y, width=507):
    p = Paragraph(text, STYLE)
    _, h = p.wrap(width, 600)
    p.drawOn(c, 44, y-h)
    return y-h-10


def supplement(board, first_page):
    stream = BytesIO()
    c = canvas.Canvas(stream, pagesize=(595.276, 841.89))
    for index, (ref, connector) in enumerate([('ADC1', 'JST-PH / ADC und GPIO'), ('J5', 'JST-SH / I2C')]):
        c.setFillColor(TEXT)
        c.setFont('GuideBold', 10)
        c.drawString(44, 800, f'ENVNODE {board.upper()} | {MARKER}')
        c.setFont('Guide', 9)
        c.setFillColor(MUTED)
        c.drawString(44, 783, 'Dokumentergänzung 18. Oktober 2026 | Hardware-Revision unverändert')
        c.setFillColor(TEXT)
        c.setFont('GuideBold', 20)
        c.drawString(44, 747, connector)
        y = paragraph(c, 'Die beiden Mini-Ausschnitte ADC1 und J5 dienen als <b>Referenz für alle entsprechenden '
                      'vierpoligen JST-PH- und JST-SH-Anschlüsse auf Mini und Weather</b>. '
                      'Signal, Bus, Referenzname und Einbaurichtung richten sich nach dem jeweiligen Anschluss.', 725)
        # Both complementary views remain readable at document scale.
        for x, view in [(44, 'oblique'), (302, 'top')]:
            c.drawImage(str(IMAGES / f'{ref}-{view}-annotated.png'), x, y-285,
                        width=247, height=285.285, preserveAspectRatio=True, mask='auto')
        y -= 306
        if ref == 'ADC1':
            rows = [('1', 'GND'), ('2', '+5V (Sensorversorgung)'), ('3', 'ADC- bzw. GPIO-Signal'), ('4', '+3V3_SYS')]
            explanation = ('JST-PH B4B-PH-K, Raster 2,00 mm. In der gezeigten Draufsicht: Pin 1 oben, '
                           'Pin 4 unten. Der orange Kreis markiert den ersten Steckkontakt. '
                           'Der Original-Silkscreen GPIO bezeichnet hier das Mini-ADC1-Signal an GPIO34.')
            scope = ('Mini: ADC1 und ADC2. Weather: ADC1 bis ADC4 sowie GPIO14, GPIO16, GPIO17, '
                     'GPIO27, GPIO32 und GPIO4. Die 5-V-Versorgung erlaubt keinen 5-V-Signalpegel '
                     'am ESP32; die Grenzen des jeweiligen Eingangs gelten weiterhin.')
        else:
            rows = [('1', 'GND'), ('2', '+3V3_SYS'), ('3', 'SDA des jeweiligen I2C-Busses'), ('4', 'SCL des jeweiligen I2C-Busses')]
            explanation = ('JST-SH BM04B-SRSS-TB, Raster 1,00 mm. In der gezeigten Draufsicht: '
                           'Pin 1 links, Pin 4 rechts. Der orange Kreis markiert den ersten Lötanschluss. '
                           'J5 gehört zu I2C0 (SDA0/SCL0); an I2C1 gilt die gleiche Folge für SDA1/SCL1. '
                           'Die seitlichen Metalllaschen sind mechanische Befestigungen, keine Signalpins.')
            scope = ('Mini: J4/J5 auf I2C0 und J6/J7 auf I2C1. Weather: I2C0_1 bis I2C0_4 und '
                     'I2C1_1 bis I2C1_4. Die Draufsicht zeigt auch die Referenz J5; in der '
                     'Schrägansicht wird diese durch das Gehäuse verdeckt.')
        for pin, signal in rows:
            c.setFillColor(HexColor('#eef2f5'))
            c.rect(44, y-20, 507, 22, fill=1, stroke=0)
            c.setFillColor(TEXT)
            c.setFont('GuideBold', 10)
            c.drawString(54, y-13, f'Pin {pin}')
            c.setFont('Guide', 10)
            c.drawString(128, y-13, signal)
            y -= 24
        y -= 10
        y = paragraph(c, explanation, y)
        y = paragraph(c, scope, y)
        if board == 'Weather':
            y = paragraph(c, '<b>Beschriftungsstand:</b> Die Bilder zeigen Mini. Weather 0.2.1 ergänzt entsprechende '
                          'Beschriftungen. Position und Einbaurichtung folgen dem jeweiligen Weather-Anschluss.', y)
        y = paragraph(c, 'Ansicht von der <b>Bestückungsseite</b>, nicht von der Kontaktseite eines Kabelsteckers. '
                      'Bei gedrehten Anschlüssen Pin 1 am tatsächlichen Footprint zuordnen. '
                      'Der zweipolige ORing-Power-Eingang ist von dieser Referenz nicht erfasst.', y)
        if y < 77:
            raise RuntimeError(f'Page overflow: {board} {ref}, y={y}')
        c.setFillColor(MUTED)
        c.setFont('Guide', 7)
        c.drawString(44, 59, 'Originale PCB-/STEP-Daten, virtuelle PCB-Schnittkanten. Mini-Pins gegen PCB und Schaltplan geprüft.')
        c.drawString(44, 48, 'Alle entsprechenden Mini-/Weather-Padbelegungen verglichen; historische Prüfstände bleiben erhalten.')
        c.drawString(44, 37, 'EnvNode: siehe Root-Lizenz LICENSE. Modelle: Rene Poeschl (2017), KiCAD (2025), CC BY-SA 4.0 mit KiCad-Ausnahme.')
        c.drawRightString(551, 24, str(first_page+index))
        c.showPage()
    c.save()
    stream.seek(0)
    return PdfReader(stream)


def main():
    for board, folder, filename in [
        ('Mini', 'EnvNode Mini', 'EnvNode_Mini_Bestueckungs_und_Bringup_Rev0.8.pdf'),
        ('Weather', 'Weatherstation', 'EnvNode_Weather_Bestueckungs_und_Bringup_Rev0.2.pdf'),
    ]:
        path = ROOT / 'hardware/kicad/MainBoards' / folder / 'Documentation' / filename
        reader = PdfReader(path)
        historical = [p for p in reader.pages if MARKER not in p.extract_text()]
        writer = PdfWriter()
        for page in historical:
            writer.add_page(page)
        for page in supplement(board, len(historical)+1).pages:
            writer.add_page(page)
        metadata = dict(reader.metadata or {})
        metadata.update({'/Title': f'EnvNode {board} - Bestückung und Bring-up mit Anschlussreferenzen',
                         '/Subject': 'Anschlussreferenzen ergänzt am 18. Oktober 2026; ursprüngliche Prüfstände unverändert',
                         '/Keywords': 'EnvNode, JST-PH, JST-SH, Pin 1, LicenseRef-EnvNode-Use-and-Redistribution-1.0'})
        writer.add_metadata(metadata)
        with BytesIO() as out:
            writer.write(out)
            path.write_bytes(out.getvalue())
        check = PdfReader(path)
        assert len(check.pages) == len(historical)+2
        for old, new in zip(historical, check.pages):
            assert old.extract_text() == new.extract_text()
        print(f'{board}: {len(historical)} retained + 2 reference pages = {len(check.pages)} pages')

if __name__ == '__main__':
    main()
