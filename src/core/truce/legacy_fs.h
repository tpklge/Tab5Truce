#pragma once
#include <LittleFS.h>
#ifdef TRUCE_SD_ONLY
#include <SD.h>
#include <truce_vfs.h>
// Compatibility for existing modules: their old LittleFS paths resolve on SD.
// begin/format/end never mount, format or unmount any flash partition.
class TruceLegacyFS : public FS {
public:
    TruceLegacyFS() : FS(std::make_shared<TruceVFS>()) { _impl->mountpoint("/sd/truce"); }
    bool begin(bool = false, const char * = nullptr, uint8_t = 3, const char * = nullptr) { return false; }
    bool format() { return false; }
    void end() {}
    size_t totalBytes() { return SD.totalBytes(); }
    size_t usedBytes() { return SD.usedBytes(); }
};
extern TruceLegacyFS truceLegacyFS;
#define LittleFS truceLegacyFS
#endif
