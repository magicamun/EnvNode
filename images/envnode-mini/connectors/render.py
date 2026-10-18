#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# EnvNode-owned script; see ../../../LICENSE. Third-party models retain their licenses.
"""Rebuild actual KiCad connector renders; requires KiCad 10 and Pillow.
Run from any directory. --annotate-only reuses the existing raw PNGs.
"""
import argparse, base64, hashlib, json, math, os, re, subprocess, tempfile
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont, PngImagePlugin
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
BOARD=ROOT/'hardware/kicad/MainBoards/EnvNode Mini/EnvNode Mini.kicad_pcb'
CLI=os.environ.get('KICAD_CLI','/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli')
MODELS=Path(os.environ.get('KICAD10_3DMODEL_DIR','/Applications/KiCad/KiCad.app/Contents/SharedSupport/3dmodels'))
SPECS={'ADC1':{'bounds':(177,112,192.372,127),'title':'ADC1 · JST-PH · 2.00 mm','pins':['GND','+5V','/ADC/ADC1','+3V3_SYS'],'top':(.740,.300),'oblique':(.740,.180)},'J5':{'bounds':(182,94,192.372,107),'title':'J5 · JST-SH · 1.00 mm','pins':['GND','+3V3_SYS','I2C0_SDA','I2C0_SCL'],'top':(.446,.550),'oblique':(.446,.524)}}
def parse(s):
 stack=[]; root=[]
 for t in re.findall(r'"(?:\\.|[^"\\])*"|[()]|[^\s()]+',s):
  if t=='(':
   a=[]
   if stack:stack[-1].append(a)
   else:root=a
   stack.append(a)
  elif t==')':stack.pop()
  else:stack[-1].append(json.loads(t) if t.startswith('"') else t)
 return root
def children(a,k):return [v for v in a if isinstance(v,list) and v and v[0]==k]
def one(a,k):return next(iter(children(a,k)),None)
def chunks(s):
 depth=0; quoted=False; escaped=False
 for i,c in enumerate(s):
  if quoted:
   if escaped:escaped=False
   elif c=='\\':escaped=True
   elif c=='"':quoted=False
   continue
  if c=='"':quoted=True;continue
  if c=='(':
   depth+=1
   if depth==2:start=i
  elif c==')':
   if depth==2:yield s[start:i+1]
   depth-=1

def crop(s,b):
 # Virtual rectangular section only; all retained footprint/model data is verbatim.
 out=[]
 for c in chunks(s):
  if c.startswith('(footprint'):
   at=re.search(r'\(at ([\d.-]+) ([\d.-]+)',c);x,y=map(float,at.groups())
   if not(b[0]-2<x<b[2]+2 and b[1]-2<y<b[3]+2):continue
  if re.search(r'\(layer "Edge.Cuts"\)',c):continue
  out.append(c)
 out.append(f'(gr_rect (start {b[0]} {b[1]}) (end {b[2]} {b[3]}) (stroke (width 0.05) (type solid)) (fill no) (layer "Edge.Cuts"))')
 return ('(kicad_pcb\n'+'\n'.join(out)+'\n)').replace('${KICAD10_3DMODEL_DIR}',str(MODELS))

def annotate(ref,view,spec):
 raw=HERE/f'{ref}-{view}.png'; im=Image.open(raw).convert('RGB').resize((2000,2000))
 canvas=Image.new('RGB',(2000,2310),'#f4f6f8');canvas.paste(im,(0,180));d=ImageDraw.Draw(canvas)
 font='/System/Library/Fonts/Supplemental/Arial.ttf'
 def f(n):return ImageFont.truetype(font,n)
 d.text((55,25),spec['title'],font=f(56),fill='#182938')
 d.text((55,98),'Draufsicht · Bestückungsseite' if view=='top' else 'Schrägansicht · Bestückungsseite',font=f(34),fill='#40556a')
 x,y=spec[view];x*=2000;y=y*2000+180
 # Overlay outside the silkscreen; tip targets pin 1 (PH contact / SH solder lead).
 color='#ff6d00'; lx=80 if ref=='J5' else 1200; ly=215
 d.rounded_rectangle((lx-12,ly-8,lx+310,ly+54),radius=10,fill='#182938')
 d.text((lx,ly),'PIN 1 · GND',font=f(40),fill='#ffac63')
 d.line([(lx+160,ly+62),(lx+160,y),(x-33,y)],fill=color,width=7)
 d.ellipse((x-29,y-29,x+29,y+29),outline=color,width=7)
 footer='1 GND   ·   2 +5V   ·   3 ADC1   ·   4 +3V3_SYS' if ref=='ADC1' else '1 GND   ·   2 +3V3_SYS   ·   3 SDA0   ·   4 SCL0'
 d.text((55,2200),footer,font=f(37),fill='#182938')
 d.text((55,2260),'KiCad 10 · Original-STEP und PCB · virtuelle PCB-Schnittkanten',font=f(26),fill='#40556a')
 meta=PngImagePlugin.PngInfo();meta.add_text('License','LicenseRef-EnvNode-Use-and-Redistribution-1.0; see LICENSE');meta.add_text('Description','Actual KiCad geometry; pin 1 annotation is a documentation overlay, not board silkscreen.')
 canvas.save(HERE/f'{ref}-{view}-annotated.png',pnginfo=meta)
 # Self-contained SVG preserves editable labels, with the native render embedded.
 encoded=base64.b64encode(raw.read_bytes()).decode()
 title=spec['title'];subtitle='Draufsicht' if view=='top' else 'Schrägansicht'
 svg=f'''<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="2000" height="2310" viewBox="0 0 2000 2310"><!-- EnvNode: see ../../../LICENSE. Original KiCad model licenses retained; see README.md. --><rect width="2000" height="2310" fill="#f4f6f8"/><image x="0" y="180" width="2000" height="2000" xlink:href="data:image/png;base64,{encoded}"/><g font-family="Arial,sans-serif" fill="#182938"><text x="55" y="75" font-size="56">{title}</text><text x="55" y="133" font-size="34">{subtitle} · Bestückungsseite</text><text x="55" y="2236" font-size="37">{footer}</text><text x="55" y="2285" font-size="26">Original-STEP und PCB · virtuelle PCB-Schnittkanten</text></g><rect x="{lx-12}" y="{ly-8}" width="322" height="62" rx="10" fill="#182938"/><text x="{lx}" y="{ly+40}" font-family="Arial,sans-serif" font-size="40" fill="#ffac63">PIN 1 · GND</text><path d="M {lx+160},{ly+62} V {y} H {x-33}" fill="none" stroke="{color}" stroke-width="7"/><circle cx="{x}" cy="{y}" r="29" fill="none" stroke="{color}" stroke-width="7"/></svg>'''
 (HERE/f'{ref}-{view}-annotated.svg').write_text(svg)

def main():
 ap=argparse.ArgumentParser();ap.add_argument('--annotate-only',action='store_true');args=ap.parse_args()
 source=BOARD.read_text(); board=parse(source);report={'board':str(BOARD.relative_to(ROOT)),'board_sha256':hashlib.sha256(BOARD.read_bytes()).hexdigest(),'renderer':subprocess.check_output([CLI,'--version'],text=True).strip(),'connectors':{}}
 with tempfile.TemporaryDirectory(prefix='envnode-connectors-') as tmp:
  netfile=Path(tmp)/'schematic.net'
  subprocess.run([CLI,'sch','export','netlist','--output',str(netfile),str(BOARD.with_suffix('.kicad_sch'))],check=True)
  nets=children(one(parse(netfile.read_text()),'nets'),'net')
  for ref,spec in SPECS.items():
   fp=next(f for f in children(board,'footprint') if any(v[1:3]==['Reference',ref] for v in children(f,'property')))
   pcb={p[1]:one(p,'net')[-1] for p in children(fp,'pad') if p[1] in ['1','2','3','4']}
   sch={one(n,'pin')[1]:one(net,'name')[1] for net in nets for n in children(net,'node') if one(n,'ref')[1]==ref}
   expected={str(i+1):n for i,n in enumerate(spec['pins'])}
   assert pcb==sch==expected,(ref,pcb,sch,expected)
   models=[]
   for m in children(fp,'model'):
    path=Path(m[1].replace('${KICAD10_3DMODEL_DIR}',str(MODELS)))
    models.append({'file':path.name,'sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'transform':m[2:]})
   report['connectors'][ref]={'footprint':fp[1],'at':one(fp,'at')[1:],'pins':pcb,'schematic_matches_pcb':True,'models':models,'virtual_crop_mm':spec['bounds']}
   if not args.annotate_only:
    section=Path(tmp)/f'{ref}.kicad_pcb';section.write_text(crop(source,spec['bounds']))
    for view in ['top','oblique']:
     cmd=[CLI,'pcb','render','--width','2000','--height','2000','--quality','high','--background','opaque','--light-top','0.35','--light-camera','0.25','--output',str(HERE/f'{ref}-{view}.png')]
     if view=='oblique':cmd+=['--rotate','332,0,0']
     subprocess.run(cmd+[str(section)],check=True)
   for view in ['top','oblique']:annotate(ref,view,spec)
 (HERE/'provenance.json').write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
