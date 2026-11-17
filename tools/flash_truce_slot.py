#!/usr/bin/env python3
"""Update only a Launcher OTA app slot, resolving its address from the device.
Never uses cached addresses, a combined image, or the standalone partition CSV.
"""
import argparse,struct,subprocess,sys,tempfile,json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
def partition_rows(data):
 rows=[]
 for off in range(0,0xc00,32):
  r=data[off:off+32]
  if r[:2]!=b'\xaa\x50':break
  _,kind,sub,address,size,label,flags=struct.unpack('<HBBII16sI',r)
  rows.append(dict(kind=kind,subtype=sub,address=address,size=size,name=label.split(b'\0')[0].decode()))
 return rows
def target_slot(rows,slot,app_size):
 applications=[r for r in rows if r['kind']==0 and 0x10<=r['subtype']<=0x1f]
 candidates=[r for r in applications if slot is None or r['subtype']==0x10+slot]
 if len(candidates)!=1:raise ValueError('Choose an existing OTA slot explicitly; found '+str([r['name'] for r in candidates]))
 target=candidates[0];start=target['address'];end=start+target['size']
 if start%4096 or target['size']%4096 or app_size>target['size'] or end>0x1000000:raise ValueError('Application does not fit the current slot')
 for r in rows:
  if r is not target and start<r['address']+r['size'] and r['address']<end:raise ValueError('Overlapping partitions')
 return target
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--port',required=True);p.add_argument('--slot',type=int)
 p.add_argument('--image',type=Path,default=ROOT/'.pio/build/m5stack-tab5/firmware.bin')
 p.add_argument('--dry-run',action='store_true');a=p.parse_args()
 app=a.image.read_bytes()
 if not app or app[0]!=0xe9 or app[32:36]!=b'2T\xcd\xab':raise SystemExit('Use the application-only image, not the combined USB image.')
 tool=[sys.executable,'-m','esptool','--chip','esp32p4','--port',a.port,'--baud','921600']
 with tempfile.TemporaryDirectory(prefix='truce-slot-') as tmp:
  table=Path(tmp)/'partitions.bin'
  subprocess.run(tool+['read-flash','0x8000','0x1000',str(table)],check=True)
  target=target_slot(partition_rows(table.read_bytes()),a.slot,len(app))
  print(json.dumps(target,indent=2),flush=True)
  if not a.dry_run:subprocess.run(tool+['write-flash',hex(target['address']),str(a.image)],check=True)
if __name__=='__main__':main()
