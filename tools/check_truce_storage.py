#!/usr/bin/env python3
"""Audit the Tab5 partition binary and linked entry points after a build."""
import argparse, json, re, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--report',type=Path,default=ROOT/'artifacts/truce-ui/storage-audit.json')
a=p.parse_args()
rows=[]
for line in (ROOT/'boards/m5stack-tab5/truce_partitions.csv').read_text().splitlines():
 if line.strip() and not line.startswith('#'):
  cols=[x.strip() for x in line.split(',')];rows.append(cols)
assert len(rows)==1 and rows[0][1:3]==['app','factory'],rows
build=ROOT/'.pio/build/m5stack-tab5'
# Inspect the binary partition table, not only its CSV source.
blob=(build/'partitions.bin').read_bytes()
entries=[]
for off in range(0,len(blob),32):
 entry=blob[off:off+32]
 if entry[:2]!=b'\xaa\x50':break
 assert entry[2]==0,'data partition in flashed table'
 entries.append(entry[12:28].split(b'\0')[0].decode())
assert entries==['factory'],entries
packages=Path.home()/'.platformio/packages'
nm=packages/'toolchain-riscv32-esp/bin/riscv32-esp-elf-nm'
text=subprocess.check_output([str(nm),'--defined-only',str(build/'firmware.elf')],text=True)
symbols={line.split()[-1] for line in text.splitlines() if line.split()}
# These are wrapped at link time; active callers cannot reach the SDK persistence entry points.
required={'__wrap_nvs_flash_init','__wrap_nvs_open','__wrap_esp_partition_write','__wrap_esp_partition_erase_range'}
missing=required-symbols
assert not missing,missing
assert 'nvs_flash_init' not in symbols,'real NVS initialization is linked'
assert 'nvs_open' not in symbols,'real NVS namespace opening is linked'
assert 'truceLegacyFS' in symbols,'SD compatibility facade missing'
# The Tab5 ES/P4 v1.x must not link the ECO3 SDK (that image can reset before setup).
link_map=(build/'firmware.map').read_text()
assert 'esp32p4_es' in link_map, 'Tab5 linked the wrong P4 silicon SDK'
sdk=(packages/'framework-arduinoespressif32-libs/esp32p4_es/dio_qspi/include/sdkconfig.h').read_text()
assert re.search(r'#define CONFIG_ESP32P4_REV_MAX_FULL\s+199\b',sdk)
report=dict(chip_variant='esp32p4_es',supported_chip_revision='0.1–1.99',partitions=entries,nvs_partitions=0,filesystem_partitions=0,coredump_partitions=0,
            initialized_nvs=False,local_flash_write_entry_points='blocked by linker guards',
            filesystem_root='/truce',legacy_filesystem='SD facade',hardware_tested=False)
a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
