#!/usr/bin/env python3
"""Validate shipped SD themes, contrast and icon confinement without extra packages."""
import json,re,struct
from pathlib import Path
root=Path(__file__).resolve().parents[1]/'sdcard/truce'
def luminance(value):
 rgb=[int(value[i:i+2],16)/255 for i in (1,3,5)]
 return sum(weight*(c/12.92 if c<=.04045 else ((c+.055)/1.055)**2.4) for c,weight in zip(rgb,(.2126,.7152,.0722)))
def contrast(a,b):
 values=sorted([luminance(a),luminance(b)])
 return (values[1]+.05)/(values[0]+.05)
icons=set();themes=[]
for path in sorted((root/'themes').glob('*.json')):
 doc=json.loads(path.read_text())
 for key in ['background','panel','text','muted','border','accent']:
  assert re.fullmatch('#[0-9a-fA-F]{6}',doc[key]),(path,key)
 assert contrast(doc['text'],doc['panel'])>=4.5,path
 assert contrast(doc['accent'],doc['panel'])>=3,path
 for value in doc['icons'].values():
  icon=(path.parent/value).resolve();assert icon.is_relative_to(root.resolve()),icon
  data=icon.read_bytes();assert data[:8]==b'\x89PNG\r\n\x1a\n',icon
  assert struct.unpack('>IIBBBBB',data[16:29])==(128,128,8,6,0,0,0),icon
  icons.add(icon)
 themes.append(doc['name'])
shark=(root/'boot/shark.png').read_bytes()
assert shark[:8]==b'\x89PNG\r\n\x1a\n'
assert struct.unpack('>IIBBBBB',shark[16:29])==(640,320,8,6,0,0,0)
for size in (20,24,32):
 font=root/'fonts'/f'latin{size}.bin'
 assert font.exists() and 1024<font.stat().st_size<128*1024
assert (root/'fonts/OFL.txt').exists()
assert 1024<(root/'fonts/digits64.bin').stat().st_size<128*1024
state=json.loads((root/'config/ui.json').read_text())
assert (root/state['theme'].lstrip('/')).exists()
print(f'{len(themes)} SD themes; {len(icons)} RGBA icons; text/accent contrast and /truce asset confinement passed.')
