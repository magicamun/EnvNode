#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../LICENSE.
"""Replace/add the ORing Power testpoint supplement, preserving historical pages."""
from io import BytesIO
from pathlib import Path
from pypdf import PdfReader,PdfWriter
from reportlab.pdfgen import canvas
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
ROOT=Path(__file__).resolve().parents[1]
PDF=ROOT/'hardware/kicad/Power/ORing Power/Documentation/ORing_Power_Bestueckungs_und_Bringup_Rev0.1.pdf'
MARKER='PRÜFPUNKTE AN DER AUSSENKANTE / ERGÄNZUNG'

def main():
    reader=PdfReader(PDF)
    historical=[p for p in reader.pages if MARKER not in p.extract_text()]
    stream=BytesIO()
    pdfmetrics.registerFont(TTFont('Guide','/System/Library/Fonts/Supplemental/Arial.ttf'))
    c=canvas.Canvas(stream,pagesize=(841.89,595.276))
    c.setFont('Guide',13)
    c.setFillColorRGB(.12,.21,.28)
    c.drawString(32,564,'ORING POWER | '+MARKER)
    c.setFont('Guide',10)
    c.drawString(32,544,'18. Oktober 2026 | PCB-Beschriftungsstand 0.1.1 | Anleitung 0.1 weiterhin gültig')
    c.drawImage(str(ROOT/'images/oring-power/testpoints/ORing-Power-testpoints.png'),32,95,width=778,height=427.9)
    c.setFont('Guide',9)
    c.drawString(32,74,'Ergänzung zu Abschnitt 5 und 6: Spannungen gegen U2.1 messen; Quellen nur gemäß dem beschriebenen Testaufbau anschließen.')
    c.drawString(32,59,'Diese Abbildung ergänzt die Orientierung. Die vollständigen Bestückungs-, Durchgangs- und Rückspeisungsprüfungen bleiben maßgeblich.')
    c.setFont('Guide',8)
    c.drawString(32,39,'EnvNode: siehe Root-Lizenz LICENSE. KiCad-Modelle behalten ihre Lizenzen; siehe images/oring-power/testpoints/THIRD_PARTY_NOTICES.txt.')
    c.drawRightString(810,23,str(len(historical)+1))
    c.showPage();c.save();stream.seek(0)
    writer=PdfWriter()
    for p in historical:writer.add_page(p)
    writer.add_page(PdfReader(stream).pages[0])
    meta=dict(reader.metadata or {})
    meta['/Subject']='Prüfpunktübersicht ergänzt am 18. Oktober 2026; historische Anleitung und Prüfstände unverändert'
    writer.add_metadata(meta)
    out=BytesIO();writer.write(out);PDF.write_bytes(out.getvalue())
    check=PdfReader(PDF)
    assert len(check.pages)==len(historical)+1
    for a,b in zip(historical,check.pages):assert a.extract_text()==b.extract_text()
    print(f'{len(historical)} historical pages retained; testpoint overview added as page {len(check.pages)}')
if __name__=='__main__':main()
