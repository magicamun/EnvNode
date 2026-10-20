#!/usr/bin/env python3
# SPDX-License-Identifier: LicenseRef-EnvNode-Use-and-Redistribution-1.0
# See ../../LICENSE. Native KiCad renders; no replacement geometry.
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import subprocess
root=Path(__file__).resolve().parents[2]
def run(job):
 name,rev,side=job
 out=root/f'images/rain-detector/RainDetector{name}-Rev{rev}-{side}.png'
 cmd=['/Applications/KiCad/KiCad.app/Contents/MacOS/kicad-cli','pcb','render','--width','2200','--height','1500','--zoom','0.8','--light-top','0.6','--light-camera','0.5','--quality','high','--background','transparent','--perspective','--rotate',('-25,0,5' if side=='top' else '-25,0,-5'),'--side',side,'--output',str(out),str(root/f'hardware/kicad/Sensors/RainDetector{name}/RainDetector{name}.kicad_pcb')]
 p=subprocess.run(cmd,capture_output=True,text=True);print(name,side,p.returncode,p.stdout[-300:],p.stderr[-300:],flush=True)
 if p.returncode:raise RuntimeError(name)
with ThreadPoolExecutor(max_workers=2) as pool:list(pool.map(run,[(n,r,s) for n,r in [('Head','0.2'),('Controller','0.4')] for s in ['top','bottom']]))
# White-background product images for the corresponding PCB articles.
from PIL import Image, ImageDraw, ImageFont
for name,rev,part in [('Head','0.2','ENP-00139'),('Controller','0.4','ENP-00138')]:
    directory=root/'images/rain-detector'
    raw=Image.open(directory/f'RainDetector{name}-Rev{rev}-top.png').convert('RGBA')
    raw=raw.crop(raw.getbbox());raw.thumbnail((1320,1000))
    product=Image.new('RGB',(1500,1200),'white')
    product.paste(raw,((1500-raw.width)//2,(1060-raw.height)//2+20),raw)
    font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',34)
    ImageDraw.Draw(product).text((750,1120),f'{part} | RainDetector{name} | Rev. {rev}',anchor='mm',font=font,fill='#243746')
    product.save(directory/f'{part}-RainDetector{name}-Rev{rev}.png')
