#!/usr/bin/env python3
"""Build the original Truce line-icon set and editable microSD theme packs (stdlib only)."""
import json, math, struct, zlib
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1] / 'sdcard' / 'truce'
PALETTES = {
 'graphite': dict(name='Grafite', background='#151B23', panel='#243041', text='#F8FAFC', muted='#CBD5E1', border='#64748B', accent='#FF6B83'),
 'ocean': dict(name='Oceano', background='#0B1B2D', panel='#17354D', text='#F1F9FF', muted='#B8D8ED', border='#5D849D', accent='#67D8EF'),
 'light': dict(name='Claro', background='#EDF1F7', panel='#FFFFFF', text='#142238', muted='#526176', border='#B7C2D3', accent='#C02640'),
}
NAMES={'WiFi':'wifi','BLE':'bluetooth','RF':'radio','NRF24':'radio','LoRa':'antenna','FM':'radio','IR':'remote','Ethernet':'network','GPS':'gps','RFID':'card','Files':'folder','JS Interpreter':'script','Clock':'clock','Others':'apps','Wire Tools':'wire','Config':'settings'}
def icon(kind, color):
 size=128; data=bytearray(size*size*4)
 def dot(x,y,r=3):
  for yy in range(max(0,int(y-r-1)),min(size,int(y+r+2))):
   for xx in range(max(0,int(x-r-1)),min(size,int(x+r+2))):
    a=max(0,min(1,r+.5-math.hypot(xx-x,yy-y)))
    if a:
     p=(yy*size+xx)*4;data[p:p+4]=bytes((*color,max(data[p+3],round(a*255))))
 def line(*points):
  for (x,y),(u,v) in zip(points,points[1:]):
   steps=max(1,int(math.hypot(u-x,v-y)*2))
   for i in range(steps+1):dot(x+(u-x)*i/steps,y+(v-y)*i/steps,2.4)
 def arc(x,y,r,a,b):
  points=[(x+r*math.cos(math.radians(t)),y+r*math.sin(math.radians(t))) for t in range(a,b+1,2)]
  line(*points)
 def box(x,y,w,h):line((x,y),(x+w,y),(x+w,y+h),(x,y+h),(x,y))
 if kind=='wifi':
  for r in (23,40,57):arc(64,94,r,225,315)
  dot(64,94,5)
 elif kind=='bluetooth':line((46,37),(85,79),(64,99),(64,25),(85,46),(46,88))
 elif kind in ('radio','antenna'):
  line((64,101),(64,64));dot(64,58,5)
  for r in (21,37):arc(64,58,r,135,225);arc(64,58,r,-45,45)
  line((49,102),(79,102))
 elif kind=='folder':line((20,43),(20,96),(108,96),(108,43),(65,43),(54,31),(20,31),(20,43))
 elif kind=='clock':arc(64,64,42,0,360);line((64,37),(64,64),(82,77))
 elif kind=='gps':
  arc(64,51,30,180,360);line((34,51),(36,67),(64,106),(92,67),(94,51));arc(64,51,10,0,360)
 elif kind=='card':box(21,37,86,58);box(34,52,19,18);line((68,58),(94,58));line((68,74),(84,74))
 elif kind=='network':box(48,22,32,24);line((64,46),(64,66),(27,66),(27,82));line((64,66),(101,66),(101,82));box(12,82,30,23);box(86,82,30,23)
 elif kind=='remote':box(44,29,40,77);dot(64,44,5);dot(57,65,3);dot(73,65,3);dot(57,83,3);dot(73,83,3);arc(64,29,17,225,315)
 elif kind=='script':line((48,40),(25,64),(48,87));line((80,40),(103,64),(80,87));line((70,31),(57,97))
 elif kind=='wire':line((26,26),(26,53),(44,70),(83,70),(101,88),(101,103));box(14,22,24,17);box(89,91,24,17);dot(64,70,6)
 elif kind=='settings':
  arc(64,64,25,0,360);arc(64,64,10,0,360)
  for t in range(0,360,45):
   a=math.radians(t);line((64+30*math.cos(a),64+30*math.sin(a)),(64+43*math.cos(a),64+43*math.sin(a)))
 else:
  for x in (28,74):
   for y in (28,74):box(x,y,26,26)
 def chunk(tag,body):return struct.pack('>I',len(body))+tag+body+struct.pack('>I',zlib.crc32(tag+body)&0xffffffff)
 rows=b''.join(b'\0'+bytes(data[y*size*4:(y+1)*size*4]) for y in range(size))
 return b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',size,size,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(rows,9))+chunk(b'IEND',b'')
for theme,palette in PALETTES.items():
 folder=ROOT/'themes'/'icons'/theme;folder.mkdir(parents=True,exist_ok=True)
 color=tuple(bytes.fromhex(palette['accent'][1:]))
 for kind in set(NAMES.values()):(folder/f'{kind}.png').write_bytes(icon(kind,color))
 content={**palette,'icons':{name:f'icons/{theme}/{kind}.png' for name,kind in NAMES.items()}}
 (ROOT/'themes'/f'{theme}.json').write_text(json.dumps(content,ensure_ascii=False,indent=2)+'\n')
(ROOT/'config').mkdir(exist_ok=True)
(ROOT/'config'/'ui.json').write_text(json.dumps({'theme':'/themes/light.json'},indent=2)+'\n')
print('Created 3 themes and',len(set(NAMES.values()))*3,'PNG icons in',ROOT)
