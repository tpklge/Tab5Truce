#!/usr/bin/env python3
"""Package the verified firmware and microSD directory without changing any device."""
from pathlib import Path
import hashlib,json,zipfile,shutil
root=Path(__file__).resolve().parents[1]
out=root/'dist';out.mkdir(exist_ok=True)
firmware=root/'Bruce-m5stack-tab5.bin'
app=root/'.pio/build/m5stack-tab5/firmware.bin'
audit=root/'artifacts/truce-ui/storage-audit.json'
if not audit.exists() or not firmware.exists():raise SystemExit('Build firmware and run check_truce_storage.py first.')
shutil.copyfile(app,out/'Truce-Tab5-app.bin')
boot_report=root/'artifacts/truce-ui/boot-hardware.json'
boot=json.loads(boot_report.read_text()) if boot_report.exists() else {}
boot_tested=boot.get('application_sha256')==hashlib.sha256(app.read_bytes()).hexdigest() and boot.get('main_menu_reached',False)
manifest={'hardware_boot_tested':bool(boot_tested),'launcher_firmware':'Truce-Tab5-app.bin','launcher_sha256':hashlib.sha256(app.read_bytes()).hexdigest(),
          'chip_variant':'esp32p4_es','supported_chip_revision':'0.1–1.99','firmware':'Truce-Tab5.bin','sha256':hashlib.sha256(firmware.read_bytes()).hexdigest(),
          'flash_offset':'0x0','microSD_root':'/truce','lvgl':'9.3.0','hardware_tested':False}
with zipfile.ZipFile(out/'Truce-Tab5.zip','w',zipfile.ZIP_DEFLATED) as z:
 z.write(firmware,'Truce-Tab5.bin')
 z.write(app,'Truce-Tab5-app.bin')
 for f in sorted((root/'sdcard').rglob('*')):
  if f.is_file():z.write(f,'microSD/'+f.relative_to(root/'sdcard').as_posix())
 z.write(root/'TRUCE_TAB5.md','LEIA-ME.md');z.write(audit,'storage-audit.json')
 if boot_tested:z.write(boot_report,'boot-hardware.json')
 for name in ('screen-inventory.json','logic-analyzer.png','all-screens-hardware.json','wifi-networks.png','bluetooth-devices.png','confirmation.png','terminal.png','performance.json','simulation.txt','screen-migration.json','gps-panel.png','ir-panel.png','reader-portrait.png','document.png','spectrum.png','audio-portrait.png','clock-portrait.png','timer.png','fm.png','megalodon.png','waveform.png'):
  report=root/'artifacts/truce-ui'/name
  if report.exists():z.write(report,name)
 z.writestr('manifest.json',json.dumps(manifest,indent=2)+'\n')
print(out/'Truce-Tab5.zip')
