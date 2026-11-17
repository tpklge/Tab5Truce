#pragma once
#include <string.h>
namespace truce {
// FAT has no symlinks. Reject traversal before the VFS appends its private mountpoint.
inline bool safePath(const char *path) {
    if (!path || path[0] != '/' || strchr(path, '\\') || strchr(path, ':')) return false;
    for (const char *p = path; *p;) {
        while (*p == '/') ++p;
        const char *start = p;
        while (*p && *p != '/') {
            if ((unsigned char)*p < 32 || (unsigned char)*p == 127) return false;
            ++p;
        }
        if (p != start && (p[-1] == '.' || p[-1] == ' ')) return false;
        if ((p - start == 2 && start[0] == '.' && start[1] == '.') ||
            (p - start == 1 && start[0] == '.')) return false;
    }
    return true;
}
}
