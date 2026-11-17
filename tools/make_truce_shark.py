#!/usr/bin/env python3
"""Original code-drawn shark mascot; transparent RGBA, stored only on microSD."""
from pathlib import Path
import struct,zlib,math
w,h=640,320
pixels=bytearray(w*h*4)
def polygon(points,color):
 for y in range(h):
  intersections=[]
  for (x1,y1),(x2,y2) in zip(points,points[1:]+points[:1]):
   if (y1<=y<y2) or (y2<=y<y1):intersections.append(x1+(y-y1)*(x2-x1)/(y2-y1))
  intersections.sort()
  for a,b in zip(intersections[::2],intersections[1::2]):
   for x in range(max(0,math.ceil(a)),min(w,math.ceil(b))):
    p=(y*w+x)*4;pixels[p:p+4]=bytes(color)
def circle(cx,cy,r,color):
 for y in range(cy-r,cy+r+1):
  for x in range(cx-r,cx+r+1):
   if (x-cx)**2+(y-cy)**2<=r*r:
    p=(y*w+x)*4;pixels[p:p+4]=bytes(color)
polygon([(110,125),(170,145),(210,112),(285,95),(320,38),(337,99),(410,115),(485,151),(510,171),(479,188),(420,194),(358,203),(325,247),(307,202),(244,205),(183,183),(110,217),(143,171)],(37,99,235,255))
polygon([(177,168),(245,183),(324,181),(397,177),(477,170),(501,173),(478,185),(418,191),(356,200),(306,198),(244,201),(184,179)],(186,230,253,255))
polygon([(307,184),(344,189),(325,238)],(30,64,175,255))
circle(450,150,8,(248,250,252,255));circle(452,150,4,(15,23,42,255))
for x in (375,386,397):polygon([(x,143),(x+3,142),(x-1,168),(x-4,169)],(15,23,42,255))
polygon([(463,175),(499,171),(491,178),(465,180)],(15,23,42,255))
for x in (470,480,490):polygon([(x,175),(x+5,174),(x+3,180)],(255,255,255,255))
def chunk(t,b):return struct.pack('>I',len(b))+t+b+struct.pack('>I',zlib.crc32(t+b)&0xffffffff)
data=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',w,h,8,6,0,0,0))+chunk(b'IDAT',zlib.compress(b''.join(b'\0'+pixels[y*w*4:(y+1)*w*4] for y in range(h)),9))+chunk(b'IEND',b'')
p=Path(__file__).resolve().parents[1]/'sdcard/truce/boot/shark.png';p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data);print(p,len(data),'bytes')
