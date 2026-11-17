#pragma once
namespace truce {
// Preserve the previous committed version until the new file has been promoted.
// Recover deliberately ignores orphan .tmp files, which may be incomplete.
template <class FS, class Path> bool recover(FS &fs, const Path &path) {
    if (fs.exists(path)) return true;
    return fs.exists(path + ".bak") && fs.rename(path + ".bak", path);
}
template <class FS, class Path> bool commit(FS &fs, const Path &tmp, const Path &path) {
    if (!fs.exists(tmp)) return false;
    const Path backup = path + ".bak";
    if (fs.exists(path)) {
        if (fs.exists(backup) && !fs.remove(backup)) return false;
        if (!fs.rename(path, backup)) return false;
    }
    if (fs.rename(tmp, path)) return true;
    recover(fs, path);
    return false;
}
}
