#ifndef USE_SD_MMC
#include "../SD.h"
#include "FS.h"
#include "ff.h"
// Copyright 2015-2016 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "sd_diskio2.h"
#include "vfs_api.h"
#ifdef TRUCE_SD_ONLY
#include "truce_vfs.h"
#endif

using namespace fs;

SDFS::SDFS(FSImplPtr impl) : FS(impl), _pdrv(0xFF) {}

bool SDFS::begin(
    uint8_t ssPin, SPIClass &spi, uint32_t frequency, const char *mountpoint, uint8_t max_files,
    bool format_if_empty
) {
    if (_pdrv != 0xFF) { return true; }

    spi.begin();

    _pdrv = sdcard_init(ssPin, &spi, frequency);
    if (_pdrv == 0xFF) { return false; }

    if (!sdcard_mount(_pdrv, mountpoint, max_files, format_if_empty)) {
        sdcard_unmount(_pdrv);
        sdcard_uninit(_pdrv);
        _pdrv = 0xFF;
        return false;
    }

#ifdef TRUCE_SD_ONLY
    // Mount the card physically at /sd, expose only /sd/truce to every FS consumer.
    _impl->mountpoint(mountpoint);
    if (!exists("/truce") && !mkdir("/truce")) { end(); return false; }
    _impl->mountpoint("/sd/truce");
#else
    _impl->mountpoint(mountpoint);
#endif
    return true;
}

void SDFS::end() {
    if (_pdrv != 0xFF) {
        _impl->mountpoint(NULL);
        sdcard_unmount(_pdrv);

        sdcard_uninit(_pdrv);
        _pdrv = 0xFF;
    }
}

sdcard_type_t SDFS::cardType() {
    if (_pdrv == 0xFF) { return CARD_NONE; }
    return sdcard_type(_pdrv);
}

uint64_t SDFS::cardSize() {
    if (_pdrv == 0xFF) { return 0; }
    size_t sectors = sdcard_num_sectors(_pdrv);
    size_t sectorSize = sdcard_sector_size(_pdrv);
    return (uint64_t)sectors * sectorSize;
}

size_t SDFS::numSectors() {
    if (_pdrv == 0xFF) { return 0; }
    return sdcard_num_sectors(_pdrv);
}

size_t SDFS::sectorSize() {
    if (_pdrv == 0xFF) { return 0; }
    return sdcard_sector_size(_pdrv);
}

uint64_t SDFS::totalBytes() {
    FATFS *fsinfo;
    DWORD fre_clust;
    char drv[3] = {(char)(48 + _pdrv), ':', 0};
    if (f_getfree(drv, &fre_clust, &fsinfo) != 0) return 0;
    uint64_t size = ((uint64_t)(fsinfo->csize)) * (fsinfo->n_fatent - 2)
#if _MAX_SS != 512
                    * (fsinfo->ssize);
#else
                    * 512;
#endif
    return size;
}

uint64_t SDFS::usedBytes() {
    FATFS *fsinfo;
    DWORD fre_clust;
    char drv[3] = {(char)(48 + _pdrv), ':', 0};
    if (f_getfree(drv, &fre_clust, &fsinfo) != 0) return 0;
    uint64_t size = ((uint64_t)(fsinfo->csize)) * ((fsinfo->n_fatent - 2) - (fsinfo->free_clst))
#if _MAX_SS != 512
                    * (fsinfo->ssize);
#else
                    * 512;
#endif
    return size;
}

bool SDFS::readRAW(uint8_t *buffer, uint32_t sector) {
#ifdef TRUCE_SD_ONLY
    return false;
#else
    return sd_read_raw(_pdrv, buffer, sector);
#endif
}

bool SDFS::writeRAW(uint8_t *buffer, uint32_t sector) {
#ifdef TRUCE_SD_ONLY
    return false; // Raw USB block access would bypass the /truce boundary.
#else
    return sd_write_raw(_pdrv, buffer, sector);
#endif
}

#ifdef TRUCE_SD_ONLY
SDFS SD = SDFS(FSImplPtr(new TruceVFS()));
#else
SDFS SD = SDFS(FSImplPtr(new VFSImpl()));
#endif
#endif
