#pragma once
#include <vfs_api.h>
#include <truce_path.h>
class TruceVFS : public VFSImpl {
public:
    FileImplPtr open(const char *path, const char *mode, bool create) override {
        return truce::safePath(path) ? VFSImpl::open(path, mode, create) : FileImplPtr();
    }
    bool exists(const char *p) override { return truce::safePath(p) && VFSImpl::exists(p); }
    bool mkdir(const char *p) override { return truce::safePath(p) && VFSImpl::mkdir(p); }
    bool remove(const char *p) override { return truce::safePath(p) && strcmp(p, "/") && VFSImpl::remove(p); }
    bool rmdir(const char *p) override { return truce::safePath(p) && strcmp(p, "/") && VFSImpl::rmdir(p); }
    bool rename(const char *a, const char *b) override {
        return truce::safePath(a) && truce::safePath(b) && strcmp(a, "/") && strcmp(b, "/") && VFSImpl::rename(a, b);
    }
};
