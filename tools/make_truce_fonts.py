#!/usr/bin/env python3
"""Rebuild microSD Latin supplements and instrument digits using lv_font_conv 1.5.3."""
import argparse,subprocess,shutil
from pathlib import Path
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--node',default='node');p.add_argument('--converter',required=True)
a=p.parse_args();root=Path(__file__).resolve().parents[1]
fonts=root/'.pio/libdeps/m5stack-tab5/lvgl/scripts/built_in_font'
out=root/'sdcard/truce/fonts';out.mkdir(parents=True,exist_ok=True)
for size in (20,24,32):
 subprocess.run([a.node,a.converter,'--size',str(size),'--bpp','4','--format','bin','--font',str(fonts/'Montserrat-Medium.ttf'),'-r','0xA0-0x17F,0x300-0x36F,0x2010-0x2027','--no-compress','--no-kerning','-o',str(out/f'latin{size}.bin')],check=True)
subprocess.run([a.node,a.converter,'--size','64','--bpp','4','--format','bin','--font',str(fonts/'Montserrat-Medium.ttf'),'-r','0x20,0x2E,0x30-0x3A,0x48,0x4D,0x7A','--no-compress','--no-kerning','-o',str(out/'digits64.bin')],check=True)
shutil.copyfile(fonts/'font_license/Montserrat/OFL.txt',out/'OFL.txt')
