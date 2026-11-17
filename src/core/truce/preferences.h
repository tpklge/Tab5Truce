#pragma once
#include <Preferences.h>
#ifdef TRUCE_SD_ONLY
#include "core/sd_functions.h"
#include <ctype.h>
#include <truce_transaction.h>
// Small binary keys on microSD; compatible with the Preferences subset used by Bruce.
class TrucePreferences {
    String base;
    bool readonly = true;
    bool valid(const char *s) const {
        if (!s || !*s || strlen(s) > 32) return false;
        for (; *s; ++s) if (!isalnum((unsigned char)*s) && *s != '_' && *s != '-') return false;
        return true;
    }
public:
    bool begin(const char *ns, bool ro = false, const char * = nullptr) {
        if (!sdcardMounted || !valid(ns)) return false;
        base = String("/preferences/") + ns;
        readonly = ro;
        if (!ro) { SD.mkdir("/preferences"); SD.mkdir(base); }
        return SD.exists(base);
    }
    void end() { base = ""; }
    size_t getBytes(const char *key, void *out, size_t len) {
        if (!base.length() || !valid(key)) return 0;
        String path = base + "/" + key;
        truce::recover(SD, path);
        File f = SD.open(path, FILE_READ);
        return f && f.size() <= len ? f.read((uint8_t *)out, f.size()) : 0;
    }
    size_t putBytes(const char *key, const void *data, size_t len) {
        if (readonly || !base.length() || !valid(key)) return 0;
        String path = base + "/" + key, tmp = path + ".tmp", bak = path + ".bak";
        File f = SD.open(tmp, FILE_WRITE);
        if (!f) return 0;
        size_t wrote = f.write((const uint8_t *)data, len);
        f.flush(); f.close();
        if (wrote != len) { SD.remove(tmp); return 0; }
        return truce::commit(SD, tmp, path) ? len : 0;
    }
    uint32_t getUInt(const char *key, uint32_t def = 0) { uint32_t v; return getBytes(key, &v, sizeof(v)) == sizeof(v) ? v : def; }
    size_t putUInt(const char *key, uint32_t v) { return putBytes(key, &v, sizeof(v)); }
    uint32_t getULong(const char *key, uint32_t def = 0) { return getUInt(key, def); }
    size_t putULong(const char *key, uint32_t v) { return putUInt(key, v); }
    uint8_t getUChar(const char *key, uint8_t def = 0) { uint8_t v; return getBytes(key, &v, sizeof(v)) == sizeof(v) ? v : def; }
    size_t putUChar(const char *key, uint8_t v) { return putBytes(key, &v, sizeof(v)); }
    String getString(const char *key, const char *def = "") {
        if (!base.length() || !valid(key)) return def;
        String path = base + "/" + key;
        truce::recover(SD, path);
        File f = SD.open(path, FILE_READ);
        return f ? f.readString() : String(def);
    }
    size_t putString(const char *key, const String &v) { return putBytes(key, v.c_str(), v.length()); }
};
#define Preferences TrucePreferences
#endif
