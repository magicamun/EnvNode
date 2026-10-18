#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../../../LICENSE. Uses original KiCad PCB/STEP render, no replacement geometry.
"""Build the documented testpoint overview from ORing-Power-top.png (fixed KiCad 10 top camera).
Render: kicad-cli pcb render --width 2600 --height 1400 --zoom 1.5 --quality high
--background transparent --light-top 0.35 --light-camera 0.25 --output ORing-Power-top.png <pcb>
Pixel anchors are calibrated to the actual rectangular outline; recheck after camera/layout changes.
"""
from pathlib import Path
import hashlib,json
from PIL import Image,ImageDraw,ImageFont,PngImagePlugin
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
raw=Image.open(HERE/'ORing-Power-top.png').convert('RGBA')
image=Image.new('RGBA',(3000,1650),'#f4f6f8')
crop=(245,270,2330,1110);scale=1900/2085
image.alpha_composite(raw.crop(crop).resize((1900,round(840*scale))), (550,240))
d=ImageDraw.Draw(image)
font=Path('/System/Library/Fonts/Supplemental')
def f(size,bold=False):return ImageFont.truetype(str(font/('Arial Bold.ttf' if bold else 'Arial.ttf')),size)
def text(x,y,t,size=34,color='#203648',bold=False):d.text((x,y),t,font=f(size,bold),fill=color)
text(65,38,'ORing Power 0.1.1 · Prüfpunkte an der Außenkante',64,bold=True)
text(65,120,'Bestückungsseite · USB-C unten · U2.1 bis U2.5 sind die fünf Randpads',36)
# Outline calibration: left x=274, right x=2302, top y=299; 52 px/mm.
# Original PCB outline: (109.082,93.410) to (148.082,108.410) mm.
def point(side,board_y):
 x=296 if side=='left' else 2280 # exposed copper just inside the half hole
 y=299+(board_y-93.410)*52
 return (550+(x-crop[0])*scale,240+(y-crop[1])*scale)
rows=[('left',100.898,'U2.1 · GND','Bezug für alle Messungen','#2d5266'),
      ('left',97.148,'U2.2 · +5V','5-V-Eingang / gemeinsame Leitung','#c66a14'),
      ('right',97.148,'U2.5 · +3V3_SYS','Ausgang hinter D1 / D2','#a0408b'),
      ('right',100.898,'U2.4 · +3V3_DEVKIT','Eingang vom ESP32-DevKit','#287da2'),
      ('right',104.648,'U2.3 · +3V3_EXT','Reglerausgang vor D1','#33805f')]
for side,y,title,subtitle,color in rows:
 px,py=point(side,y);left=55 if side=='left' else 2470
 width=455
 d.rounded_rectangle((left,py-57,left+width,py+64),radius=16,fill='white',outline=color,width=3)
 text(left+17,py-43,title,37,color,True)
 # split manually to keep long label within the side panel
 if side=='left' and '5-V-' in subtitle:
  text(left+17,py+4,'5-V-Eingang',28);text(left+17,py+34,'gemeinsame +5V-Leitung',25)
 else:text(left+17,py+10,subtitle,25)
 line_start=left+width if side=='left' else left
 line_end=px-21 if side=='left' else px+21
 d.line((line_start,py,line_end,py),fill=color,width=6)
 d.ellipse((px-21,py-21,px+21,py+21),outline=color,width=6)
text(65,1050,'Einzelmodul prüfen · alle Spannungen gegen U2.1 / GND',44,bold=True)
for y,pin,expect in [(1125,'U2.2  +5V','5,0 V Eingang'),(1190,'U2.3  +3V3_EXT','ca. 3,3 V vom Regler'),(1255,'U2.5  +3V3_SYS','etwas unter 3,3 V: Spannungsabfall über D1')]:
 text(80,y,pin,36,bold=True);text(690,y,expect,36)
text(80,1338,'Bei reiner 5-V-Versorgung wird U2.4 nicht aktiv gespeist; dort keinen 3,3-V-Sollwert voraussetzen.',32)
text(80,1392,'Separater DevKit-Pfadtest: +5V aus, strombegrenzte potentialfreie 3,3 V an U2.4; U2.5 = 3,3 V minus D2.',32)
text(80,1450,'Erster Test ohne Mainboard: nur eine 5-V-Quelle, zunächst ca. 100 mA Strombegrenzung (wie Anleitung).',32)
text(65,1530,'PCB-Pads und Schaltplan-Netzliste abgeglichen. J1 ohne 3D-Modell: nur Original-Footprint sichtbar.',27,color='#526779')
text(65,1575,'Originale KiCad-Geometrie · Pinmarkierungen sind Dokumentations-Overlays · Lizenz: siehe LICENSE',27,color='#526779')
meta=PngImagePlugin.PngInfo();meta.add_text('License','LicenseRef-EnvNode-Use-and-Redistribution-1.0; see ../../../LICENSE')
image.convert('RGB').save(HERE/'ORing-Power-testpoints.png',pnginfo=meta)
board=ROOT/'hardware/kicad/Power/ORing Power/ORing Power.kicad_pcb'
(HERE/'provenance.json').write_text(json.dumps({'board':str(board.relative_to(ROOT)),'sha256':hashlib.sha256(board.read_bytes()).hexdigest(),'renderer':'KiCad 10.0.5','view':'top, component side, USB-C bottom','pads':{t.split(' · ')[0]:t.split(' · ')[1] for _,_,t,_,_ in rows},'schematic_netlist_matches_pcb':True,'missing_model':'J1; original PCB footprint shown without connector housing'},indent=2)+'\n')
